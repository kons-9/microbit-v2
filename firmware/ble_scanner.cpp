/*
 * ble_scanner.cpp — BLE scanner implementation
 */
#include "ble_scanner.h"
#include "arch/arch.h"

#include <cstring>
#include <cstdio>

namespace ble {

int BleScanner::add_beacon(const char *name, float x, float y)
{
    if (name == nullptr) return -1;
    if (n_beacons_ >= MAX_BEACONS) return -1;

    auto &b = beacons_[n_beacons_];
    std::memset(b.name, 0, sizeof(b.name));
    std::strncpy(b.name, name, sizeof(b.name) - 1);
    b.known_x = x;
    b.known_y = y;
    b.active  = true;
    n_beacons_++;
    return n_beacons_ - 1;
}

int BleScanner::scan(ScanResult &result, int samples_override)
{
    int n_samples = (samples_override > 0) ? samples_override
                                            : SAMPLES_PER_SCAN;
    result.n_beacons = n_beacons_;

    for (int i = 0; i < n_beacons_; i++) {
        result.beacon_x[i] = beacons_[i].known_x;
        result.beacon_y[i] = beacons_[i].known_y;

        /* Average multiple RSSI samples for stability */
        int32_t sum = 0;
        int     valid = 0;
        for (int s = 0; s < n_samples; s++) {
            int8_t rssi = 0;
            int rc = arch::ble_scan_rssi(beacons_[i].name, rssi);
            if (rc != 0) {
                continue;
            }
            sum += rssi;
            valid++;
        }
        result.rssi[i] = (valid > 0)
            ? static_cast<int8_t>(sum / valid)
            : static_cast<int8_t>(-100);
    }

    return 0;
}

int BleScanner::scan_filtered(ScanResult &result, int samples_override,
                              int8_t rssi_min, int8_t rssi_max)
{
    int n_samples = (samples_override > 0) ? samples_override
                                            : SAMPLES_PER_SCAN;
    result.n_beacons = n_beacons_;

    for (int i = 0; i < n_beacons_; i++) {
        result.beacon_x[i] = beacons_[i].known_x;
        result.beacon_y[i] = beacons_[i].known_y;

        int32_t sum = 0;
        int     valid = 0;
        for (int s = 0; s < n_samples; s++) {
            int8_t rssi = 0;
            int rc = arch::ble_scan_rssi(beacons_[i].name, rssi);
            if (rc != 0) continue;
            /* Discard out-of-range readings */
            if (rssi < rssi_min || rssi > rssi_max) continue;
            sum += rssi;
            valid++;
        }
        result.rssi[i] = (valid > 0)
            ? static_cast<int8_t>(sum / valid)
            : static_cast<int8_t>(-100);
    }

    return 0;
}

} // namespace ble
