#include "ota.h"
#include "arch/ota_arch.h"
#include "ble.h"

#include <string.h>

/* ---- Internal State ---- */

static ota_state_t s_state = OTA_STATE_IDLE;
static ota_progress_fn s_progress_cb = NULL;

static ota_image_header_t s_header;
static uint32_t s_received_bytes;

/* ---- CRC32 (simple implementation) ---- */

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len) {
    crc = ~crc;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

/* ---- API Implementation ---- */

int ota_start_receive(void) {
    s_state = OTA_STATE_IDLE;
    s_received_bytes = 0;
    s_progress_cb = NULL;
    memset(&s_header, 0, sizeof(s_header));

    s_state = OTA_STATE_RECEIVING;

    /* TODO: BLE GATTサービス起動 & 受信ループ
     *
     * 実装方針:
     * 1. OTA用GATTサービスを登録 (OTA_SERVICE_UUID)
     * 2. Control Point characteristic で header 受信
     * 3. Data characteristic で chunk 受信
     * 4. 各chunk を Flash に書き込み
     * 5. 全データ受信後 CRC32 検証
     */

    /* placeholder: 実際はBLE GATTコールバックで駆動される */
    return OTA_OK;
}

void ota_set_progress_callback(ota_progress_fn fn) {
    s_progress_cb = fn;
}

ota_state_t ota_get_state(void) {
    return s_state;
}

int ota_switch_mode(ota_boot_mode_t mode) {
    if (mode == OTA_BOOT_APP && s_state != OTA_STATE_COMPLETE) {
        return OTA_ERR_STATE;
    }
    ota_arch_reboot(mode);
    /* not reached */
    return OTA_OK;
}

void ota_reset(void) {
    s_state = OTA_STATE_IDLE;
    s_received_bytes = 0;
    memset(&s_header, 0, sizeof(s_header));
}

/* ---- Internal: chunk受信ハンドラ (GATTコールバックから呼ばれる) ---- */

int ota_on_header_received(const ota_image_header_t *hdr) {
    if (s_state != OTA_STATE_RECEIVING) {
        return OTA_ERR_STATE;
    }

    if (hdr->magic != OTA_IMAGE_MAGIC) {
        s_state = OTA_STATE_ERROR;
        return OTA_ERR_CHECKSUM;
    }

    if (hdr->image_size > ota_arch_get_app_slot_size()) {
        s_state = OTA_STATE_ERROR;
        return OTA_ERR_SIZE;
    }

    memcpy(&s_header, hdr, sizeof(s_header));
    s_received_bytes = 0;

    /* アプリスロット消去 */
    ota_arch_flash_erase(0, s_header.image_size);

    return OTA_OK;
}

int ota_on_data_received(const uint8_t *data, uint32_t len) {
    if (s_state != OTA_STATE_RECEIVING) {
        return OTA_ERR_STATE;
    }

    if (s_received_bytes + len > s_header.image_size) {
        s_state = OTA_STATE_ERROR;
        return OTA_ERR_SIZE;
    }

    ota_arch_flash_write(s_received_bytes, data, len);

    s_received_bytes += len;

    if (s_progress_cb) {
        s_progress_cb(s_received_bytes, s_header.image_size);
    }

    /* 全データ受信完了 → 検証 */
    if (s_received_bytes >= s_header.image_size) {
        s_state = OTA_STATE_VERIFYING;

        /* CRC32検証 */
        uint32_t crc = 0;
        uint8_t buf[256];
        uint32_t offset = 0;
        uint32_t remaining = s_header.image_size;

        while (remaining > 0) {
            uint32_t chunk = (remaining < sizeof(buf)) ? remaining : sizeof(buf);
            ota_arch_flash_read(offset, buf, chunk);
            crc = crc32_update(crc, buf, chunk);
            offset += chunk;
            remaining -= chunk;
        }

        if (crc != s_header.crc32) {
            s_state = OTA_STATE_ERROR;
            return OTA_ERR_CHECKSUM;
        }

        s_state = OTA_STATE_COMPLETE;
    }

    return OTA_OK;
}
