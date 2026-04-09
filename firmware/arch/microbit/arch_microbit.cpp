/*
 * arch/microbit/arch_microbit.cpp — micro:bit hardware backend (stub)
 *
 * micro:bit v2.2 (nRF52833) operates in dual BLE role:
 *   - Observer: scans BLE beacons for RSSI values
 *   - Peripheral: GATT server notifies smartphone of estimated position
 *
 * TODO: Implement with real BLE radio when micro:bit board
 *       and μT-Kernel 3.0 BSP are available.
 */
#include "../arch.h"

#ifdef UAI_PLATFORM_MICROBIT

// #include <tk/tkernel.h>
// #include <nrf_sdh_ble.h>

namespace ble {
namespace arch {

/* --- Platform init --- */

int init()
{
    /* TODO: Initialize nRF SoftDevice (BLE stack)
       - sd_softdevice_enable()
       - Configure observer role (scanning)
       - Configure peripheral role (GATT server) */
    return -1;
}

/* --- BLE Observer: beacon scanning --- */

int ble_scan_rssi(const char *beacon_name, int8_t &out_rssi)
{
    /* TODO: Perform active BLE scan for beacon_name,
       read RSSI from scan response.
       - sd_ble_gap_scan_start() with filter
       - Match advertised name, extract RSSI */
    (void)beacon_name;
    (void)out_rssi;
    return -1;
}

/* --- BLE Peripheral: GATT server --- */

static ModeWriteCallback mode_cb_ = nullptr;

int gatt_init()
{
    /* TODO: Register GATT service with characteristics:
       - Position characteristic (notify): x(f32), y(f32), conf(f32)
       - Mode characteristic (read/write): mode(u8)
       UUID base: custom 128-bit, e.g. BLE_LOC_SERVICE_UUID */
    return -1;
}

int gatt_notify_position(float x, float y, float confidence)
{
    /* TODO: Pack (x, y, confidence) into 12-byte value
       - sd_ble_gatts_hvx() with BLE_GATT_HVX_NOTIFICATION */
    (void)x; (void)y; (void)confidence;
    return -1;
}

int gatt_notify_mode(uint8_t mode)
{
    /* TODO: Notify mode characteristic value change */
    (void)mode;
    return -1;
}

bool gatt_is_connected()
{
    /* TODO: Check BLE connection state */
    return false;
}

void gatt_set_mode_callback(ModeWriteCallback cb)
{
    mode_cb_ = cb;
    /* TODO: Register BLE_GATTS_EVT_WRITE handler
       that calls mode_cb_ when mode characteristic is written */
}

/* --- Platform timer --- */

uint32_t millis()
{
    /* TODO: Use tk_get_tim() or RTC */
    return 0;
}

void sleep_ms(uint32_t ms)
{
    /* TODO: tk_dly_tsk(ms) */
    (void)ms;
}

} // namespace arch
} // namespace ble

#endif /* UAI_PLATFORM_MICROBIT */
