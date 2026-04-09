/*
 * ble_scanner.h — BLE RSSI scanner
 *
 * Collects RSSI (Received Signal Strength Indicator) values
 * from known BLE beacons. Prepares data for AI inference.
 */
#pragma once

#include "app_config.h"

#include <cstdint>

namespace ble {

struct BeaconInfo {
    char     name[16];     /* beacon identifier (e.g. "Beacon-A") */
    float    known_x;      /* beacon's known X position (meters) */
    float    known_y;      /* beacon's known Y position (meters) */
    bool     active;
};

struct ScanResult {
    int      n_beacons;
    int8_t   rssi[MAX_BEACONS];          /* latest RSSI per beacon (dBm) */
    float    beacon_x[MAX_BEACONS];      /* beacon X positions */
    float    beacon_y[MAX_BEACONS];      /* beacon Y positions */
};

class BleScanner {
public:
    BleScanner() = default;

    /* Register a known beacon at a fixed position */
    int add_beacon(const char *name, float x, float y);

    /* Perform a BLE scan and fill result with RSSI values.
       Uses arch layer for actual/simulated BLE scanning.
       samples_override: if > 0, use this instead of default SAMPLES_PER_SCAN */
    int scan(ScanResult &result, int samples_override = 0);

    /* Perform a scan with RSSI quality filtering.
       Discards readings outside [rssi_min, rssi_max] range. */
    int scan_filtered(ScanResult &result, int samples_override,
                      int8_t rssi_min = -95, int8_t rssi_max = -20);

    int beacon_count() const { return n_beacons_; }
    const BeaconInfo &beacon(int i) const { return beacons_[i]; }

private:
    BeaconInfo beacons_[MAX_BEACONS]{};
    int        n_beacons_{0};
};

} // namespace ble
