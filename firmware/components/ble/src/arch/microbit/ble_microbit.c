#include "../ble_arch.h"

/**
 * microbit (nRF52833) 実装 - placeholder
 */

int ble_arch_init(void) {
    /* TODO: RADIO peripheral init */
    return 0;
}

int ble_arch_scan_start(uint16_t interval_625us, uint16_t window_625us, int passive) {
    (void)interval_625us;
    (void)window_625us;
    (void)passive;
    /* TODO: RADIO BLE scan on ADV channels */
    return 0;
}

int ble_arch_scan_stop(void) {
    /* TODO: RADIO disable */
    return 0;
}
