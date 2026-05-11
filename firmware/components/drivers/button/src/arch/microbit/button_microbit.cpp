/**
 * @file button_microbit.cpp
 * @brief micro:bit v2.2 ボタン GPIO ポーリング実装
 */

#include "button.h"

#define LOG_TAG "BTN"
#include "log.h"

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
    LOG_D("init: A=P0.14, B=P0.23");
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
            if (auto er = tk_dly_tsk(DEBOUNCE_MS); er < E_OK) {
                LOG_E("tk_dly_tsk failed: %ld", static_cast<int32_t>(er));
                return false;
            }
            if (button_is_pressed(id)) {
                return true;
            }
        }
        if (auto er = tk_dly_tsk(POLL_INTERVAL_MS); er < E_OK) {
            LOG_E("tk_dly_tsk failed: %ld", static_cast<int32_t>(er));
            return false;
        }
        elapsed += POLL_INTERVAL_MS;
    }
    return false;
}

uint8_t button_wait_any(uint32_t timeout_ms) {
    uint32_t elapsed = 0;

    while (timeout_ms == 0 || elapsed < timeout_ms) {
        if (button_is_pressed(0)) {
            if (auto er = tk_dly_tsk(DEBOUNCE_MS); er < E_OK) {
                LOG_E("tk_dly_tsk failed: %ld", static_cast<int32_t>(er));
                return 0xFF;
            }
            if (button_is_pressed(0)) {
                return 0;
            }
        }
        if (button_is_pressed(1)) {
            if (auto er = tk_dly_tsk(DEBOUNCE_MS); er < E_OK) {
                LOG_E("tk_dly_tsk failed: %ld", static_cast<int32_t>(er));
                return 0xFF;
            }
            if (button_is_pressed(1)) {
                return 1;
            }
        }
        if (auto er = tk_dly_tsk(POLL_INTERVAL_MS); er < E_OK) {
            LOG_E("tk_dly_tsk failed: %ld", static_cast<int32_t>(er));
            return 0xFF;
        }
        elapsed += POLL_INTERVAL_MS;
    }
    return 0;
}
