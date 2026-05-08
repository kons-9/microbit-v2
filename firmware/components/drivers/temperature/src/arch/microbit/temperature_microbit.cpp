/**
 * @file temperature_microbit.cpp
 * @brief micro:bit v2.2 温度センサ (nRF52833 TEMP peripheral) 実装
 */

#include "temperature.h"

#include "nrf.h"

void temperature_init(void) {
    /* TEMP peripheral は特別な初期化不要 */
}

int32_t temperature_read_raw(void) {
    NRF_TEMP->TASKS_START = 1;

    while (NRF_TEMP->EVENTS_DATARDY == 0) {
        /* busy wait (典型的に 36μs) */
    }
    NRF_TEMP->EVENTS_DATARDY = 0;

    int32_t raw = NRF_TEMP->TEMP;

    NRF_TEMP->TASKS_STOP = 1;

    return raw;
}

int8_t temperature_read(void) {
    return static_cast<int8_t>(temperature_read_raw() / 4);
}
