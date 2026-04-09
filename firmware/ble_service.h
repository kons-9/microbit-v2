/*
 * ble_service.h — BLE GATT service for smartphone communication
 *
 * Application-layer wrapper around arch::gatt_* functions.
 * Manages BLE connection state and provides a clean interface
 * for notifying the smartphone of position updates and mode changes.
 *
 * Protocol (BLE GATT characteristics):
 *   Position (notify): 12 bytes — x(f32) y(f32) confidence(f32)
 *   Mode (read/write): 1 byte  — 0=ACCURACY, 1=RESPONSIVE
 */
#pragma once

#include "app_config.h"

#include <cstdint>

namespace ble {

class BleService {
public:
    BleService() = default;

    /* Initialize GATT server. Returns 0 on success. */
    int init();

    /* Send position update to connected smartphone.
       Returns 0 on success, -1 if no smartphone connected. */
    int notify_position(float x, float y, float confidence);

    /* Send mode change notification.
       mode: 0=ACCURACY, 1=RESPONSIVE. */
    int notify_mode(uint8_t mode);

    /* Check if smartphone is connected. */
    bool is_connected() const;

    /* Register callback for when smartphone requests mode change.
       The callback receives the requested mode (0 or 1). */
    using ModeRequestCallback = void (*)(uint8_t mode);
    void on_mode_request(ModeRequestCallback cb);

    /* Statistics */
    uint32_t notify_count() const { return notify_count_; }
    uint32_t error_count() const  { return error_count_; }

private:
    bool     initialized_{false};
    uint32_t notify_count_{0};
    uint32_t error_count_{0};
};

} // namespace ble
