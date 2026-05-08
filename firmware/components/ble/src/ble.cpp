/**
 * @file ble.cpp
 * @brief BLE 共通ロジック (プラットフォーム非依存)
 *
 * arch 層の抽象化を介して、スキャン/アドバタイズの状態管理と
 * イベントディスパッチを行う。
 */

#include "ble.h"
#include "arch/ble_arch.h"

#include <cstring>

/* ==================================================================
 * Discovery (Scanner) 内部状態
 * ================================================================== */

static int32_t s_scanning = 0;
static BLEGapEventCallback s_callback = nullptr;
static void *s_callbackArgument = nullptr;

/**
 * arch 層からの ADV パケット受信コールバック
 *
 * NOTE: RADIO 割り込みコンテキストから呼ばれる可能性がある。
 * BLEGapEvent を組み立てて、ユーザーコールバック (s_callback) に転送する。
 */
static void on_advertise_received(const ble_gap_discoveryDescriptor *descriptor) {
    if (s_callback == nullptr) {
        return;
    }

    BLEGapEvent event{};
    event.type = static_cast<uint8_t>(BLEGapEventType::Discovery);
    event.discovery = *descriptor;

    auto result = s_callback(&event, s_callbackArgument);
    if (result != 0) {
        /* ユーザーが非ゼロを返した → スキャン停止 */
        ble_gap_discover_cancel();
    }
}

/* ==================================================================
 * Discovery API 実装
 * ================================================================== */

int32_t ble_init(void) {
    s_scanning = 0;
    s_callback = nullptr;
    s_callbackArgument = nullptr;
    return ble_arch_init();
}

int32_t ble_gap_discover(uint8_t own_address_type,
                         int32_t duration_ms,
                         const ble_gap_discoveryParams *params,
                         BLEGapEventCallback callback,
                         void *callback_argument) {
    (void)own_address_type;
    (void)duration_ms;

    if (s_scanning != 0) {
        return static_cast<int32_t>(BLEError::Busy);
    }
    if (callback == nullptr || params == nullptr) {
        return static_cast<int32_t>(BLEError::InvalidParam);
    }

    s_callback = callback;
    s_callbackArgument = callback_argument;
    s_scanning = 1;

    auto result = ble_arch_scan_start(params->interval, params->window, params->is_passive, on_advertise_received);
    if (result != 0) {
        s_scanning = 0;
        return static_cast<int32_t>(BLEError::Hardware);
    }

    return static_cast<int32_t>(BLEError::Success);
}

int32_t ble_gap_discover_cancel(void) {
    if (s_scanning == 0) {
        return static_cast<int32_t>(BLEError::Success);
    }

    ble_arch_scan_stop();
    s_scanning = 0;

    /* discovery_complete イベント通知 */
    if (s_callback != nullptr) {
        BLEGapEvent event{};
        event.type = static_cast<uint8_t>(BLEGapEventType::DiscoveryComplete);
        event.discovery_complete.reason = 0;
        s_callback(&event, s_callbackArgument);
    }

    return static_cast<int32_t>(BLEError::Success);
}

int32_t ble_gap_discovery_active(void) {
    return s_scanning;
}

/* ==================================================================
 * Advertise (Broadcaster)
 * ================================================================== */

static int32_t s_advertising = 0;

/** AD データバッファ (ユーザーが設定した AD 構造体列) */
static uint8_t s_advertiseData[BLE_ADVERTISE_DATA_MAX_LENGTH] = {};
static uint8_t s_advertiseDataLength = 0;

/** 自局アドレス (Advertising で使うアドバタイザアドレス) */
static BLEAddress s_ownAddress = {};

/**
 * ADV_NONCONN_IND の PDU を構築する
 *
 * BLE ADV PDU の構造 (nRF52 RADIO 用):
 *   [S0: 1B] [LENGTH: 1B] [AdvA: 6B] [AdvData: 0-31B]
 *
 * S0 = PDU Header 下位バイト:
 *   bit[3:0] = PDU Type (ADV_NONCONN_IND = 0x02)
 *   bit[6]   = TxAdd (0=public, 1=random)
 *
 * LENGTH = AdvA(6) + AdvData の長さ
 */
static constexpr uint8_t PDU_BUFFER_SIZE = 2 + 6 + BLE_ADVERTISE_DATA_MAX_LENGTH;
static uint8_t s_pduBuffer[PDU_BUFFER_SIZE] = {};
static uint8_t s_pduLength = 0;

static void build_advertise_pdu(uint8_t advertise_type, uint8_t tx_add) {
    uint8_t payload_length = 6 + s_advertiseDataLength;

    /* S0: PDU Header 下位バイト */
    s_pduBuffer[0] = (advertise_type & 0x0F) | static_cast<uint8_t>((tx_add & 0x01) << 6);

    /* LENGTH: payload 長 */
    s_pduBuffer[1] = payload_length;

    /* AdvA: アドバタイザアドレス (6B, LSByte first) */
    std::memcpy(&s_pduBuffer[2], s_ownAddress.value, 6);

    /* AdvData */
    std::memcpy(&s_pduBuffer[8], s_advertiseData, s_advertiseDataLength);

    s_pduLength = static_cast<uint8_t>(2 + payload_length);
}

int32_t ble_gap_advertise_set_data(const uint8_t *data, uint8_t length) {
    if (data == nullptr || length > BLE_ADVERTISE_DATA_MAX_LENGTH) {
        return static_cast<int32_t>(BLEError::InvalidParam);
    }
    std::memcpy(s_advertiseData, data, length);
    s_advertiseDataLength = length;
    return static_cast<int32_t>(BLEError::Success);
}

int32_t ble_gap_advertise_start(uint8_t own_address_type, const BLEGapAdvertiseParams *params) {
    if (s_advertising != 0) {
        return static_cast<int32_t>(BLEError::Busy);
    }
    if (params == nullptr) {
        return static_cast<int32_t>(BLEError::InvalidParam);
    }

    /*
     * 自局アドレスの設定
     * TODO: FICR->DEVICEADDR から読み取る
     */
    s_ownAddress.type = own_address_type;
    s_ownAddress.value[0] = 0x01;
    s_ownAddress.value[1] = 0x02;
    s_ownAddress.value[2] = 0x03;
    s_ownAddress.value[3] = 0x04;
    s_ownAddress.value[4] = 0x05;
    s_ownAddress.value[5] = 0xC0; /* NOTE: random static の場合 bit[7:6]=11 */

    /* PDU を構築 */
    uint8_t tx_add = (own_address_type == static_cast<uint8_t>(BLEAddressType::Random)) ? 1 : 0;
    build_advertise_pdu(params->advertise_type, tx_add);

    /* arch 層に PDU を渡す */
    auto result = ble_arch_advertise_set_pdu(s_pduBuffer, s_pduLength);
    if (result != 0) {
        return static_cast<int32_t>(BLEError::Hardware);
    }

    /* Advertising 開始 (interval の中間値を使用) */
    uint16_t interval = static_cast<uint16_t>((params->interval_min + params->interval_max) / 2);
    result = ble_arch_advertise_start(interval);
    if (result != 0) {
        return static_cast<int32_t>(BLEError::Hardware);
    }

    s_advertising = 1;
    return static_cast<int32_t>(BLEError::Success);
}

int32_t ble_gap_advertise_stop(void) {
    if (s_advertising == 0) {
        return static_cast<int32_t>(BLEError::Success);
    }
    ble_arch_advertise_stop();
    s_advertising = 0;
    return static_cast<int32_t>(BLEError::Success);
}

int32_t ble_gap_advertise_active(void) {
    return s_advertising;
}
