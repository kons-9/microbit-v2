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

namespace drivers {

/* ==================================================================
 * Discovery (Scanner) 内部状態
 * ================================================================== */

/**
 * arch 層からの ADV パケット受信コールバック
 *
 * NOTE: RADIO 割り込みコンテキストから呼ばれる可能性がある。
 * インスタンスへ処理を戻して、ユーザーコールバックに転送する。
 */
void Ble::on_advertise_received(const ble::DiscoveryDescriptor *descriptor, void *argument) {
    if (argument == nullptr) {
        return;
    }

    static_cast<Ble *>(argument)->handle_advertise_received(descriptor);
}

void Ble::handle_advertise_received(const ble::DiscoveryDescriptor *descriptor) {
    if (m_state.discovery.callback == nullptr || descriptor == nullptr) {
        return;
    }

    ble::GapEvent event{};
    event.type = ble::GapEventType::Discovery;
    event.discovery = *descriptor;

    auto result = m_state.discovery.callback(&event, m_state.discovery.callback_argument);
    if (result != 0) {
        /* ユーザーが非ゼロを返した → スキャン停止 */
        gap_discover_cancel();
    }
}

/* ==================================================================
 * Discovery API 実装
 * ================================================================== */

int32_t Ble::init() {
    m_state.discovery.scanning = 0;
    m_state.discovery.callback = nullptr;
    m_state.discovery.callback_argument = nullptr;
    LOG_D("init");
    return ble_arch_init();
}

int32_t Ble::gap_discover(ble::AddressType own_address_type,
                          int32_t duration_ms,
                          const ble::DiscoveryParams *params,
                          ble::GapEventCallback callback,
                          void *callback_argument) {
    (void)own_address_type;
    (void)duration_ms;

    if (m_state.discovery.scanning != 0) {
        return static_cast<int32_t>(ble::Error::Busy);
    }
    if (callback == nullptr || params == nullptr) {
        return static_cast<int32_t>(ble::Error::InvalidParam);
    }

    m_state.discovery.callback = callback;
    m_state.discovery.callback_argument = callback_argument;
    m_state.discovery.scanning = 1;

    auto result
        = ble_arch_scan_start(params->interval, params->window, params->is_passive, &Ble::on_advertise_received, this);
    if (result != 0) {
        m_state.discovery.scanning = 0;
        LOG_E("scan start failed: %ld", result);
        return static_cast<int32_t>(ble::Error::Hardware);
    }

    LOG_D("scan started (interval=%u window=%u passive=%d)", params->interval, params->window, params->is_passive);
    return static_cast<int32_t>(ble::Error::Success);
}

int32_t Ble::gap_discover_cancel() {
    if (m_state.discovery.scanning == 0) {
        return static_cast<int32_t>(ble::Error::Success);
    }

    ble_arch_scan_stop();
    m_state.discovery.scanning = 0;
    LOG_D("scan stopped");

    /* discovery_complete イベント通知 */
    if (m_state.discovery.callback != nullptr) {
        ble::GapEvent event{};
        event.type = ble::GapEventType::DiscoveryComplete;
        event.discovery_complete.reason = 0;
        m_state.discovery.callback(&event, m_state.discovery.callback_argument);
    }

    return static_cast<int32_t>(ble::Error::Success);
}

int32_t Ble::gap_discovery_active() const {
    return m_state.discovery.scanning;
}

/* ==================================================================
 * Advertise (Broadcaster)
 * ================================================================== */

void Ble::build_advertise_pdu(ble::AdvertisePduType advertise_type, uint8_t tx_add) {
    uint8_t payload_length = 6 + m_state.advertise.advertise_data_length;
    const uint8_t pdu_type = static_cast<uint8_t>(advertise_type);

    /* S0: PDU Header 下位バイト */
    m_state.advertise.pdu_buffer[0] = (pdu_type & 0x0F) | static_cast<uint8_t>((tx_add & 0x01) << 6);

    /* LENGTH: payload 長 */
    m_state.advertise.pdu_buffer[1] = payload_length;

    /* S1: Advertising PDUではRFU (on-air 2bit、RAM上1byte) */
    m_state.advertise.pdu_buffer[2] = 0;

    /* AdvA: AdvA(6B, LSByte first) */
    std::memcpy(&m_state.advertise.pdu_buffer[3], m_state.advertise.own_address.value, 6);

    /* AdvData */
    std::memcpy(&m_state.advertise.pdu_buffer[9],
                m_state.advertise.advertise_data,
                m_state.advertise.advertise_data_length);

    m_state.advertise.pdu_length = static_cast<uint8_t>(3 + payload_length);
}

int32_t Ble::gap_advertise_set_data(const uint8_t *data, uint8_t length) {
    if (data == nullptr || length > ble::ADVERTISE_DATA_MAX_LENGTH) {
        return static_cast<int32_t>(ble::Error::InvalidParam);
    }
    std::memcpy(m_state.advertise.advertise_data, data, length);
    m_state.advertise.advertise_data_length = length;
    return static_cast<int32_t>(ble::Error::Success);
}

int32_t Ble::gap_advertise_start(ble::AddressType own_address_type, const ble::GapAdvertiseParams *params) {
    if (m_state.advertise.advertising != 0) {
        return static_cast<int32_t>(ble::Error::Busy);
    }
    if (params == nullptr) {
        return static_cast<int32_t>(ble::Error::InvalidParam);
    }

    /*
     * 自局アドレスの設定
     * TODO: FICR->DEVICEADDR から読み取る
     */
    m_state.advertise.own_address.type = own_address_type;
    m_state.advertise.own_address.value[0] = 0x01;
    m_state.advertise.own_address.value[1] = 0x02;
    m_state.advertise.own_address.value[2] = 0x03;
    m_state.advertise.own_address.value[3] = 0x04;
    m_state.advertise.own_address.value[4] = 0x05;
    m_state.advertise.own_address.value[5] = 0xC0; /* NOTE: random static の場合 bit[7:6]=11 */

    /* PDU を構築 */
    uint8_t tx_add = (own_address_type == ble::AddressType::Random) ? 1 : 0;
    build_advertise_pdu(params->advertise_type, tx_add);

    /* arch 層に PDU を渡す */
    auto result = ble_arch_advertise_set_pdu(m_state.advertise.pdu_buffer, m_state.advertise.pdu_length);
    if (result != 0) {
        return static_cast<int32_t>(ble::Error::Hardware);
    }

    LOG_D("advertise pdu: type=%u tx_add=%u addr=%02x:%02x:%02x:%02x:%02x:%02x length=%u",
          static_cast<unsigned>(static_cast<uint8_t>(params->advertise_type)),
          static_cast<unsigned>(tx_add),
          static_cast<unsigned>(m_state.advertise.own_address.value[5]),
          static_cast<unsigned>(m_state.advertise.own_address.value[4]),
          static_cast<unsigned>(m_state.advertise.own_address.value[3]),
          static_cast<unsigned>(m_state.advertise.own_address.value[2]),
          static_cast<unsigned>(m_state.advertise.own_address.value[1]),
          static_cast<unsigned>(m_state.advertise.own_address.value[0]),
          static_cast<unsigned>(m_state.advertise.pdu_length));
    logging::Logger::instance().hex_dump(
        logging::LogLevel::Debug, "BLE", m_state.advertise.pdu_buffer, m_state.advertise.pdu_length);

    /* Advertising 開始 (interval の中間値を使用) */
    uint16_t interval = static_cast<uint16_t>((params->interval_min + params->interval_max) / 2);
    result = ble_arch_advertise_start(interval);
    if (result != 0) {
        LOG_E("advertise start failed: %ld", result);
        return static_cast<int32_t>(ble::Error::Hardware);
    }

    m_state.advertise.advertising = 1;
    LOG_D("advertise started (interval=%u, data_len=%u)", interval, m_state.advertise.advertise_data_length);
    return static_cast<int32_t>(ble::Error::Success);
}

int32_t Ble::gap_advertise_get_debug_status(ble::AdvertiseDebugStatus *status) const {
    if (status == nullptr) {
        return static_cast<int32_t>(ble::Error::InvalidParam);
    }
    return ble_arch_get_advertise_debug_status(status);
}

int32_t Ble::gap_advertise_stop() {
    if (m_state.advertise.advertising == 0) {
        return static_cast<int32_t>(ble::Error::Success);
    }
    ble_arch_advertise_stop();
    m_state.advertise.advertising = 0;
    LOG_D("advertise stopped");
    return static_cast<int32_t>(ble::Error::Success);
}

int32_t Ble::gap_advertise_active() const {
    return m_state.advertise.advertising;
}

}  // namespace drivers
