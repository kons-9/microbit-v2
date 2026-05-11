/**
 * @file temperature_microbit.cpp
 * @brief micro:bit v2.2 温度センサ (nRF52833 TEMP peripheral) 実装
 */

#include "temperature.h"

#define LOG_TAG "TEMP"
#include "log.h"

#include "nrf.h"

void temperature_init(void) {
    LOG_D("init: nRF52 TEMP peripheral");
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
    int8_t temp = static_cast<int8_t>(temperature_read_raw() / 4);
    LOG_D("read: %d C", temp);
    return temp;
}
