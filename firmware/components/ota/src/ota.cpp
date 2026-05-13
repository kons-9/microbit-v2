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

namespace ota {

/* ==================================================================
 * 内部状態
 * ================================================================== */

static State s_state = State::Idle;
static ProgressCallback s_progressCallback = nullptr;
static ImageHeader s_header = {};
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

int32_t start_receive(void) {
    s_state = State::Idle;
    s_receivedBytes = 0;
    s_progressCallback = nullptr;
    std::memset(&s_header, 0, sizeof(s_header));

    s_state = State::Receiving;

    /* TODO: BLE GATT サービス起動 & 受信ループ
     *
     * 実装方針:
     * 1. OTA 用 GATT サービスを登録 (OTA_SERVICE_UUID)
     * 2. Control Point characteristic で header 受信
     * 3. Data characteristic で chunk 受信
     * 4. 各 chunk を Flash に書き込み
     * 5. 全データ受信後 CRC32 検証
     */

    return static_cast<int32_t>(Error::Ok);
}

void set_progress_callback(ProgressCallback callback) {
    s_progressCallback = callback;
}

uint8_t get_state(void) {
    return static_cast<uint8_t>(s_state);
}

int32_t switch_mode(sysconfig_boot_mode_t mode) {
    if (mode == SYSCONFIG_BOOT_APP && s_state != State::Complete) {
        return static_cast<int32_t>(Error::State);
    }
    ota_arch_reboot(mode);
    /* UNREACHABLE: ota_arch_reboot() は戻らない */
    return static_cast<int32_t>(Error::Ok);
}

void reset(void) {
    s_state = State::Idle;
    s_receivedBytes = 0;
    std::memset(&s_header, 0, sizeof(s_header));
}

/* ==================================================================
 * Internal: GATT コールバックハンドラ
 * ================================================================== */

int32_t on_header_received(const ImageHeader *header) {
    if (s_state != State::Receiving) {
        return static_cast<int32_t>(Error::State);
    }

    if (header->magic != IMAGE_MAGIC) {
        s_state = State::Error;
        return static_cast<int32_t>(Error::Checksum);
    }

    if (header->image_size > ota_arch_get_app_slot_size()) {
        s_state = State::Error;
        return static_cast<int32_t>(Error::Size);
    }

    std::memcpy(&s_header, header, sizeof(s_header));
    s_receivedBytes = 0;

    ota_arch_flash_erase(0, s_header.image_size);

    return static_cast<int32_t>(Error::Ok);
}

int32_t on_data_received(const uint8_t *data, uint32_t length) {
    if (s_state != State::Receiving) {
        return static_cast<int32_t>(Error::State);
    }

    if (s_receivedBytes + length > s_header.image_size) {
        s_state = State::Error;
        return static_cast<int32_t>(Error::Size);
    }

    ota_arch_flash_write(s_receivedBytes, data, length);
    s_receivedBytes += length;

    if (s_progressCallback != nullptr) {
        s_progressCallback(s_receivedBytes, s_header.image_size);
    }

    /* 全データ受信完了 → 検証 */
    if (s_receivedBytes >= s_header.image_size) {
        s_state = State::Verifying;

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
            s_state = State::Error;
            return static_cast<int32_t>(Error::Checksum);
        }

        s_state = State::Complete;
    }

    return static_cast<int32_t>(Error::Ok);
}

}  // namespace ota
