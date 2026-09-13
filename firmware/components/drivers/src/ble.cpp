/**
 * @file ble.cpp
 * @brief BLE 共通ロジック (プラットフォーム非依存)
 *
 * arch 層の抽象化を介して、スキャン/アドバタイズの状態管理と
 * イベントディスパッチを行う。
 */

#include "ble.h"
#include "arch/ble_arch.h"

#define LOG_TAG "BLE"
#include "log.h"

#include <cstring>

namespace ble {

/* ==================================================================
 * Discovery (Scanner) 内部状態
 * ================================================================== */

static int32_t s_scanning = 0;
static GapEventCallback s_callback = nullptr;
static void *s_callbackArgument = nullptr;

/**
 * arch 層からの ADV パケット受信コールバック
 *
 * NOTE: RADIO 割り込みコンテキストから呼ばれる可能性がある。
 * GapEvent を組み立てて、ユーザーコールバック (s_callback) に転送する。
 */
static void on_advertise_received(const DiscoveryDescriptor *descriptor) {
    if (s_callback == nullptr) {
        return;
    }

    GapEvent event{};
    event.type = static_cast<uint8_t>(GapEventType::Discovery);
    event.discovery = *descriptor;

    auto result = s_callback(&event, s_callbackArgument);
    if (result != 0) {
        /* ユーザーが非ゼロを返した → スキャン停止 */
        gap_discover_cancel();
    }
}

/* ==================================================================
 * Discovery API 実装
 * ================================================================== */

int32_t init(void) {
    s_scanning = 0;
    s_callback = nullptr;
    s_callbackArgument = nullptr;
    LOG_D("init");
    return ble_arch_init();
}

int32_t gap_discover(uint8_t own_address_type,
                     int32_t duration_ms,
                     const DiscoveryParams *params,
                     GapEventCallback callback,
                     void *callback_argument) {
    (void)own_address_type;
    (void)duration_ms;

    if (s_scanning != 0) {
        return static_cast<int32_t>(Error::Busy);
    }
    if (callback == nullptr || params == nullptr) {
        return static_cast<int32_t>(Error::InvalidParam);
    }

    s_callback = callback;
    s_callbackArgument = callback_argument;
    s_scanning = 1;

    auto result = ble_arch_scan_start(params->interval, params->window, params->is_passive, on_advertise_received);
    if (result != 0) {
        s_scanning = 0;
        LOG_E("scan start failed: %ld", result);
        return static_cast<int32_t>(Error::Hardware);
    }

    LOG_D("scan started (interval=%u window=%u passive=%d)", params->interval, params->window, params->is_passive);
    return static_cast<int32_t>(Error::Success);
}

int32_t gap_discover_cancel(void) {
    if (s_scanning == 0) {
        return static_cast<int32_t>(Error::Success);
    }

    ble_arch_scan_stop();
    s_scanning = 0;
    LOG_D("scan stopped");

    /* discovery_complete イベント通知 */
    if (s_callback != nullptr) {
        GapEvent event{};
        event.type = static_cast<uint8_t>(GapEventType::DiscoveryComplete);
        event.discovery_complete.reason = 0;
        s_callback(&event, s_callbackArgument);
    }

    return static_cast<int32_t>(Error::Success);
}

int32_t gap_discovery_active(void) {
    return s_scanning;
}

/* ==================================================================
 * Advertise (Broadcaster)
 * ================================================================== */

static int32_t s_advertising = 0;

/** AD データバッファ (ユーザーが設定した AD 構造体列) */
static uint8_t s_advertiseData[ADVERTISE_DATA_MAX_LENGTH] = {};
static uint8_t s_advertiseDataLength = 0;

/** 自局アドレス (Advertising で使うアドバタイザアドレス) */
static Address s_ownAddress = {};

/**
 * ADV_NONCONN_IND の PDU を構築する
 *
 * BLE ADV PDU の構造 (nRF52 RADIO RAM 用):
 *   [S0: 1B] [LENGTH: 1B] [S1: 1B] [AdvA: 6B] [AdvData: 0-31B]
 *
 * PCNF0.S1LEN=2 のため、S1 は on-air では2bitだが、RADIOのRAM上では
 * 1byteを占有する。Advertising PDUではS1の値はRFUなので0を設定する。
 *
 * S0 = PDU Header 下位バイト:
 *   bit[3:0] = PDU Type (ADV_NONCONN_IND = 0x02)
 *   bit[6]   = TxAdd (0=public, 1=random)
 *
 * LENGTH = AdvA(6) + AdvData の長さ
 */
static constexpr uint8_t PDU_BUFFER_SIZE = 3 + 6 + ADVERTISE_DATA_MAX_LENGTH;
static uint8_t s_pduBuffer[PDU_BUFFER_SIZE] = {};
static uint8_t s_pduLength = 0;

static void build_advertise_pdu(uint8_t advertise_type, uint8_t tx_add) {
    uint8_t payload_length = 6 + s_advertiseDataLength;

    /* S0: PDU Header 下位バイト */
    s_pduBuffer[0] = (advertise_type & 0x0F) | static_cast<uint8_t>((tx_add & 0x01) << 6);

    /* LENGTH: payload 長 */
    s_pduBuffer[1] = payload_length;

    /* S1: Advertising PDUではRFU (on-air 2bit、RAM上1byte) */
    s_pduBuffer[2] = 0;

    /* AdvA: アドバタイザアドレス (6B, LSByte first) */
    std::memcpy(&s_pduBuffer[3], s_ownAddress.value, 6);

    /* AdvData */
    std::memcpy(&s_pduBuffer[9], s_advertiseData, s_advertiseDataLength);

    s_pduLength = static_cast<uint8_t>(3 + payload_length);
}

int32_t gap_advertise_set_data(const uint8_t *data, uint8_t length) {
    if (data == nullptr || length > ADVERTISE_DATA_MAX_LENGTH) {
        return static_cast<int32_t>(Error::InvalidParam);
    }
    std::memcpy(s_advertiseData, data, length);
    s_advertiseDataLength = length;
    return static_cast<int32_t>(Error::Success);
}

int32_t gap_advertise_start(uint8_t own_address_type, const GapAdvertiseParams *params) {
    if (s_advertising != 0) {
        return static_cast<int32_t>(Error::Busy);
    }
    if (params == nullptr) {
        return static_cast<int32_t>(Error::InvalidParam);
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
    uint8_t tx_add = (own_address_type == static_cast<uint8_t>(AddressType::Random)) ? 1 : 0;
    build_advertise_pdu(params->advertise_type, tx_add);

    /* arch 層に PDU を渡す */
    auto result = ble_arch_advertise_set_pdu(s_pduBuffer, s_pduLength);
    if (result != 0) {
        return static_cast<int32_t>(Error::Hardware);
    }

    LOG_D("advertise pdu: type=%u tx_add=%u addr=%02x:%02x:%02x:%02x:%02x:%02x length=%u",
          static_cast<unsigned>(params->advertise_type),
          static_cast<unsigned>(tx_add),
          static_cast<unsigned>(s_ownAddress.value[5]),
          static_cast<unsigned>(s_ownAddress.value[4]),
          static_cast<unsigned>(s_ownAddress.value[3]),
          static_cast<unsigned>(s_ownAddress.value[2]),
          static_cast<unsigned>(s_ownAddress.value[1]),
          static_cast<unsigned>(s_ownAddress.value[0]),
          static_cast<unsigned>(s_pduLength));
    LogHexDump(LOG_LEVEL_DEBUG, "BLE", s_pduBuffer, s_pduLength);

    /* Advertising 開始 (interval の中間値を使用) */
    uint16_t interval = static_cast<uint16_t>((params->interval_min + params->interval_max) / 2);
    result = ble_arch_advertise_start(interval);
    if (result != 0) {
        LOG_E("advertise start failed: %ld", result);
        return static_cast<int32_t>(Error::Hardware);
    }

    s_advertising = 1;
    LOG_D("advertise started (interval=%u, data_len=%u)", interval, s_advertiseDataLength);
    return static_cast<int32_t>(Error::Success);
}

int32_t gap_advertise_get_debug_status(AdvertiseDebugStatus *status) {
    if (status == nullptr) {
        return static_cast<int32_t>(Error::InvalidParam);
    }
    return ble_arch_get_advertise_debug_status(status);
}

int32_t gap_advertise_stop(void) {
    if (s_advertising == 0) {
        return static_cast<int32_t>(Error::Success);
    }
    ble_arch_advertise_stop();
    s_advertising = 0;
    LOG_D("advertise stopped");
    return static_cast<int32_t>(Error::Success);
}

int32_t gap_advertise_active(void) {
    return s_advertising;
}

}  // namespace ble
