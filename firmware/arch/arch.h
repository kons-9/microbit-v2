/*
 * arch/arch.h — Architecture abstraction for BLE Locator
 *
 * Abstracts BLE scanning, BLE GATT service, and platform timer.
 * Each arch/ subdirectory provides a concrete implementation.
 *
 * The micro:bit simultaneously operates as:
 *   - BLE Observer (scanning beacons for RSSI)
 *   - BLE Peripheral (GATT server notifying smartphone of position)
 */
#pragma once

#include <cstdint>

namespace ble {
namespace arch {

/* Initialize platform (BLE radio, etc.) */
int init();

/* --- BLE Observer: beacon RSSI scanning --- */

/* Scan for a specific beacon and return its RSSI (dBm).
   beacon_name: identifier of the beacon to scan for.
   out_rssi: receives the signal strength.
   Returns 0 on success, negative if beacon not found. */
int ble_scan_rssi(const char *beacon_name, int8_t &out_rssi);

/* --- BLE Peripheral: GATT server for smartphone --- */

/* Initialize GATT server with position service.
   Returns 0 on success. */
int gatt_init();

/* Notify connected smartphone of updated position.
   x, y: estimated coordinates (meters).
   confidence: inference confidence [0.0, 1.0].
   Returns 0 on success, negative if no client connected. */
int gatt_notify_position(float x, float y, float confidence);

/* Notify connected smartphone of mode change.
   mode: 0=ACCURACY, 1=RESPONSIVE.
   Returns 0 on success. */
int gatt_notify_mode(uint8_t mode);

/* Check if a smartphone is currently connected via GATT. */
bool gatt_is_connected();

/* Callback type for mode-write from smartphone.
   Invoked when smartphone writes to mode characteristic. */
using ModeWriteCallback = void (*)(uint8_t mode);

/* Register callback for mode-write events from smartphone.
   Only one callback can be registered. */
void gatt_set_mode_callback(ModeWriteCallback cb);

/* --- Platform timer --- */

/* Get elapsed time in milliseconds */
uint32_t millis();

/* Sleep for given milliseconds */
void sleep_ms(uint32_t ms);

} // namespace arch
} // namespace ble
