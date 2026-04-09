/*
 * arch/sim/arch_sim.cpp — PC simulation backend for BLE Locator
 *
 * Simulates BLE RSSI readings based on a virtual person
 * walking around a room.
 */
#include "../arch.h"

#include <cstdio>
#include <cstring>
#include <cmath>
#include <chrono>
#include <thread>

namespace ble {
namespace arch {

static auto start_time = std::chrono::steady_clock::now();
static int  scan_count = 0;

/* Simulated person position (walks in a figure-8 pattern) */
static void get_simulated_position(float &x, float &y)
{
    float t = static_cast<float>(scan_count) * 0.3f;
    /* Figure-8 in a 10m x 8m room */
    x = 5.0f + 3.0f * std::sin(t);
    y = 4.0f + 2.5f * std::sin(2.0f * t);
}

/* Known beacon positions for simulation */
struct SimBeacon {
    const char *name;
    float x, y;
};
static const SimBeacon sim_beacons[] = {
    {"Beacon-A",  1.0f, 1.0f},
    {"Beacon-B",  9.0f, 1.0f},
    {"Beacon-C",  5.0f, 7.0f},
    {"Beacon-D",  1.0f, 7.0f},
};
static constexpr int NUM_SIM_BEACONS = 4;

int init()
{
    std::printf("[SIM] BLE Locator initialized (simulation mode)\n");
    start_time = std::chrono::steady_clock::now();
    scan_count = 0;
    return 0;
}

int ble_scan_rssi(const char *beacon_name, int8_t &out_rssi)
{
    /* Find the simulated beacon */
    const SimBeacon *found = nullptr;
    for (int i = 0; i < NUM_SIM_BEACONS; i++) {
        if (std::strcmp(beacon_name, sim_beacons[i].name) == 0) {
            found = &sim_beacons[i];
            break;
        }
    }
    if (found == nullptr) return -1;

    /* Calculate RSSI from distance using log-distance path loss model:
       RSSI = -10 * n * log10(d) + A
       where n=2.5 (indoor), A=-40 (RSSI at 1m) */
    float px, py;
    get_simulated_position(px, py);

    float dx = px - found->x;
    float dy = py - found->y;
    float dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 0.1f) dist = 0.1f;

    float rssi_f = -40.0f - 25.0f * std::log10(dist);

    /* Add noise (deterministic based on scan_count for reproducibility) */
    float noise = static_cast<float>((scan_count * 7 + 13) % 11) - 5.0f;
    rssi_f += noise;

    /* Clamp to valid BLE RSSI range */
    if (rssi_f > -20.0f) rssi_f = -20.0f;
    if (rssi_f < -100.0f) rssi_f = -100.0f;

    out_rssi = static_cast<int8_t>(rssi_f);
    scan_count++;
    return 0;
}

uint32_t millis()
{
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - start_time).count());
}

void sleep_ms(uint32_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

/* --- GATT simulation (prints to console) --- */

static bool       gatt_initialized = false;
static bool       gatt_client_connected = false;
static ModeWriteCallback mode_cb = nullptr;

int gatt_init()
{
    gatt_initialized    = true;
    gatt_client_connected = true;   /* simulate a connected smartphone */
    std::printf("[SIM] GATT server initialized (simulated smartphone connected)\n");
    return 0;
}

int gatt_notify_position(float x, float y, float confidence)
{
    if (!gatt_client_connected) return -1;
    std::printf("[SIM→Phone] Position: (%.2f, %.2f) conf=%.3f\n",
                x, y, confidence);
    return 0;
}

int gatt_notify_mode(uint8_t mode)
{
    if (!gatt_client_connected) return -1;
    std::printf("[SIM→Phone] Mode: %s\n",
                mode == 0 ? "ACCURACY" : "RESPONSIVE");
    return 0;
}

bool gatt_is_connected()
{
    return gatt_client_connected;
}

void gatt_set_mode_callback(ModeWriteCallback cb)
{
    mode_cb = cb;
}

} // namespace arch
} // namespace ble
