/**
 * @file button_microbit.cpp
 * @brief micro:bit v2.2 ボタン GPIO ポーリング実装
 */

#include "button.h"

#include "nrf_gpio.h"

#include <tk/tkernel.h>

/* ==================================================================
 * Constants
 * ================================================================== */

static constexpr uint32_t BUTTON_A_PIN = NRF_GPIO_PIN_MAP(0, 14);
static constexpr uint32_t BUTTON_B_PIN = NRF_GPIO_PIN_MAP(0, 23);

static constexpr uint32_t s_buttonPins[] = {BUTTON_A_PIN, BUTTON_B_PIN};

static constexpr uint32_t DEBOUNCE_MS = 50;
static constexpr uint32_t POLL_INTERVAL_MS = 10;

/* ==================================================================
 * API
 * ================================================================== */

void button_init(void) {
    nrf_gpio_cfg_input(BUTTON_A_PIN, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(BUTTON_B_PIN, NRF_GPIO_PIN_NOPULL);
}

bool button_is_pressed(uint8_t id) {
    if (id > 1) {
        return false;
    }
    return (nrf_gpio_pin_read(s_buttonPins[id]) == 0);
}

bool button_wait_press(uint8_t id, uint32_t timeout_ms) {
    uint32_t elapsed = 0;

    while (timeout_ms == 0 || elapsed < timeout_ms) {
        if (button_is_pressed(id)) {
            tk_dly_tsk(DEBOUNCE_MS);
            if (button_is_pressed(id)) {
                return true;
            }
        }
        tk_dly_tsk(POLL_INTERVAL_MS);
        elapsed += POLL_INTERVAL_MS;
    }
    return false;
}

uint8_t button_wait_any(uint32_t timeout_ms) {
    uint32_t elapsed = 0;

    while (timeout_ms == 0 || elapsed < timeout_ms) {
        if (button_is_pressed(0)) {
            tk_dly_tsk(DEBOUNCE_MS);
            if (button_is_pressed(0)) {
                return 0;
            }
        }
        if (button_is_pressed(1)) {
            tk_dly_tsk(DEBOUNCE_MS);
            if (button_is_pressed(1)) {
                return 1;
            }
        }
        tk_dly_tsk(POLL_INTERVAL_MS);
        elapsed += POLL_INTERVAL_MS;
    }
    return 0;
}
