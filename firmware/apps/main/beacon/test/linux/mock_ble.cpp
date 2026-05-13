/**
 * @file mock_ble.cpp
 * @brief テスト用 BLE モック — Advertising の状態をキャプチャ
 */

#include "ble.h"

#include <cstring>

/* ================================================================== */
/*  Internal state                                                    */
/* ================================================================== */

static int32_t s_initialized = 0;
static int32_t s_advertising = 0;
static uint8_t s_advData[ble::ADVERTISE_DATA_MAX_LENGTH] = {};
static uint8_t s_advDataLength = 0;
static uint16_t s_advInterval = 0;

/* ================================================================== */
/*  Test helpers                                                      */
/* ================================================================== */

extern "C" int32_t mock_ble_get_advertising() {
    return s_advertising;
}

extern "C" const uint8_t *mock_ble_get_adv_data() {
    return s_advData;
}

extern "C" uint8_t mock_ble_get_adv_data_length() {
    return s_advDataLength;
}

extern "C" uint16_t mock_ble_get_adv_interval() {
    return s_advInterval;
}

extern "C" void mock_ble_reset() {
    s_initialized = 0;
    s_advertising = 0;
    s_advDataLength = 0;
    s_advInterval = 0;
    std::memset(s_advData, 0, sizeof(s_advData));
}

/* ================================================================== */
/*  BLE API mock                                                      */
/* ================================================================== */

namespace ble {

int32_t init(void) {
    s_initialized = 1;
    s_advertising = 0;
    return 0;
}

int32_t gap_advertise_set_data(const uint8_t *data, uint8_t length) {
    if (data == nullptr || length > ADVERTISE_DATA_MAX_LENGTH) {
        return static_cast<int32_t>(Error::InvalidParam);
    }
    std::memcpy(s_advData, data, length);
    s_advDataLength = length;
    return 0;
}

int32_t gap_advertise_start(uint8_t /*own_address_type*/, const GapAdvertiseParams *params) {
    if (!s_initialized) {
        return static_cast<int32_t>(Error::Hardware);
    }
    if (s_advertising) {
        return static_cast<int32_t>(Error::Busy);
    }
    if (params == nullptr) {
        return static_cast<int32_t>(Error::InvalidParam);
    }
    s_advInterval = static_cast<uint16_t>((params->interval_min + params->interval_max) / 2);
    s_advertising = 1;
    return 0;
}

int32_t gap_advertise_stop(void) {
    s_advertising = 0;
    return 0;
}

int32_t gap_advertise_active(void) {
    return s_advertising;
}

/* Scan API (unused but needed for linkage) */
int32_t gap_discover(uint8_t, int32_t, const DiscoveryParams *, GapEventCallback, void *) {
    return 0;
}

int32_t gap_discover_cancel(void) {
    return 0;
}

int32_t gap_discovery_active(void) {
    return 0;
}

}  // namespace ble
