/**
 * @file ota.cpp
 * @brief OTA 状態管理 (プラットフォーム非依存)
 *
 * ファームウェアイメージの受信、CRC32 検証、Flash 書き込みの
 * 状態管理を行う。Flash 操作の実体は arch 層に委譲する。
 */

#include "ota.h"
#include "arch/ota_arch.h"
#include "ble.h"

#include <cstring>

/* ==================================================================
 * 内部状態
 * ================================================================== */

static OTAState s_state = OTAState::Idle;
static OTAProgressCallback s_progressCallback = nullptr;
static OTAImageHeader s_header = {};
static uint32_t s_receivedBytes = 0;

/* ==================================================================
 * CRC32 (simple implementation)
 * ================================================================== */

static uint32_t UpdateCRC32(uint32_t crc, const uint8_t *data, size_t length) {
    crc = ~crc;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int32_t j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320U & (-(crc & 1)));
        }
    }
    return ~crc;
}

/* ==================================================================
 * API 実装
 * ================================================================== */

int32_t ota_start_receive(void) {
    s_state = OTAState::Idle;
    s_receivedBytes = 0;
    s_progressCallback = nullptr;
    std::memset(&s_header, 0, sizeof(s_header));

    s_state = OTAState::Receiving;

    /* TODO: BLE GATT サービス起動 & 受信ループ
     *
     * 実装方針:
     * 1. OTA 用 GATT サービスを登録 (OTA_SERVICE_UUID)
     * 2. Control Point characteristic で header 受信
     * 3. Data characteristic で chunk 受信
     * 4. 各 chunk を Flash に書き込み
     * 5. 全データ受信後 CRC32 検証
     */

    return static_cast<int32_t>(OTAError::Ok);
}

void ota_set_progress_callback(OTAProgressCallback callback) {
    s_progressCallback = callback;
}

uint8_t ota_get_state(void) {
    return static_cast<uint8_t>(s_state);
}

int32_t ota_switch_mode(sysconfig_boot_mode_t mode) {
    if (mode == SYSCONFIG_BOOT_APP && s_state != OTAState::Complete) {
        return static_cast<int32_t>(OTAError::State);
    }
    ota_arch_reboot(mode);
    /* UNREACHABLE: ota_arch_reboot() は戻らない */
    return static_cast<int32_t>(OTAError::Ok);
}

void ota_reset(void) {
    s_state = OTAState::Idle;
    s_receivedBytes = 0;
    std::memset(&s_header, 0, sizeof(s_header));
}

/* ==================================================================
 * Internal: GATT コールバックハンドラ
 * ================================================================== */

int32_t ota_on_header_received(const OTAImageHeader *header) {
    if (s_state != OTAState::Receiving) {
        return static_cast<int32_t>(OTAError::State);
    }

    if (header->magic != OTA_IMAGE_MAGIC) {
        s_state = OTAState::Error;
        return static_cast<int32_t>(OTAError::Checksum);
    }

    if (header->image_size > ota_arch_get_app_slot_size()) {
        s_state = OTAState::Error;
        return static_cast<int32_t>(OTAError::Size);
    }

    std::memcpy(&s_header, header, sizeof(s_header));
    s_receivedBytes = 0;

    ota_arch_flash_erase(0, s_header.image_size);

    return static_cast<int32_t>(OTAError::Ok);
}

int32_t ota_on_data_received(const uint8_t *data, uint32_t length) {
    if (s_state != OTAState::Receiving) {
        return static_cast<int32_t>(OTAError::State);
    }

    if (s_receivedBytes + length > s_header.image_size) {
        s_state = OTAState::Error;
        return static_cast<int32_t>(OTAError::Size);
    }

    ota_arch_flash_write(s_receivedBytes, data, length);
    s_receivedBytes += length;

    if (s_progressCallback != nullptr) {
        s_progressCallback(s_receivedBytes, s_header.image_size);
    }

    /* 全データ受信完了 → 検証 */
    if (s_receivedBytes >= s_header.image_size) {
        s_state = OTAState::Verifying;

        uint32_t crc = 0;
        uint8_t buffer[256];
        uint32_t offset = 0;
        uint32_t remaining = s_header.image_size;

        while (remaining > 0) {
            uint32_t chunk = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);
            ota_arch_flash_read(offset, buffer, chunk);
            crc = UpdateCRC32(crc, buffer, chunk);
            offset += chunk;
            remaining -= chunk;
        }

        if (crc != s_header.crc32) {
            s_state = OTAState::Error;
            return static_cast<int32_t>(OTAError::Checksum);
        }

        s_state = OTAState::Complete;
    }

    return static_cast<int32_t>(OTAError::Ok);
}
