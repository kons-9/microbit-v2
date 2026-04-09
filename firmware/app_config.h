/*
 * app_config.h — BLE Indoor Locator application parameters
 */
#pragma once

#include <cstdint>

namespace ble {

/* Maximum number of BLE beacons to track */
inline constexpr int    MAX_BEACONS      = 8;

/* Number of RSSI samples per scan (for averaging) — default */
inline constexpr int    SAMPLES_PER_SCAN = 3;

/* BLE scan interval (ms) — default */
inline constexpr uint32_t SCAN_INTERVAL_MS = 1000;

/* Room dimensions for display (meters) */
inline constexpr float  ROOM_W           = 10.0f;
inline constexpr float  ROOM_H           = 8.0f;

/* AI model name for position estimation */
inline constexpr const char *MODEL_BLE_LOCATE = "ble_locate";

/* Inference timeout (ms) */
inline constexpr uint32_t INFER_TIMEOUT_MS = 3000;

/* Position history length */
inline constexpr int    POSITION_HISTORY  = 20;

/* --- Estimation mode parameters --- */

/* Accuracy mode: more samples, slower, position smoothing */
inline constexpr int      ACCURACY_SAMPLES           = 5;
inline constexpr uint32_t ACCURACY_SCAN_INTERVAL_MS  = 2000;
inline constexpr float    ACCURACY_SMOOTHING_ALPHA    = 0.3f;
inline constexpr float    ACCURACY_CONF_THRESHOLD     = 0.5f;

/* Responsive mode: fewer samples, faster, no smoothing */
inline constexpr int      RESPONSIVE_SAMPLES           = 1;
inline constexpr uint32_t RESPONSIVE_SCAN_INTERVAL_MS  = 500;
inline constexpr float    RESPONSIVE_SMOOTHING_ALPHA    = 1.0f;
inline constexpr float    RESPONSIVE_CONF_THRESHOLD     = 0.2f;

/* Auto-switch: streak length before mode change */
inline constexpr int    AUTO_SWITCH_STREAK = 3;

/* --- Area analysis --- */

/* Grid size for area-based dwell time tracking */
inline constexpr int    ZONE_COLS         = 5;
inline constexpr int    ZONE_ROWS         = 4;

} // namespace ble
