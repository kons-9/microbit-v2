/**
 * @file button_microbit.cpp
 * @brief micro:bit v2.2 ボタン GPIO ポーリング実装
 */

#include "button.h"

#define LOG_TAG "BTN"
#include "log.h"

#include "nrf_gpio.h"

#include <utkernel/task>

/* ==================================================================
 * Constants
 * ================================================================== */

static constexpr uint32_t BUTTON_A_PIN = NRF_GPIO_PIN_MAP(0, 14);
static constexpr uint32_t BUTTON_B_PIN = NRF_GPIO_PIN_MAP(0, 23);

static constexpr uint32_t BUTTON_PINS[] = {BUTTON_A_PIN, BUTTON_B_PIN};

static constexpr uint32_t DEBOUNCE_MS = 50;
static constexpr uint32_t POLL_INTERVAL_MS = 10;

namespace drivers {

static bool is_valid_button(ButtonId id) {
    return id == ButtonId::A || id == ButtonId::B;
}

/* ==================================================================
 * Public API
 * ================================================================== */

void Button::init() {
    nrf_gpio_cfg_input(BUTTON_A_PIN, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(BUTTON_B_PIN, NRF_GPIO_PIN_NOPULL);
    LOG_D("init: A=P0.14, B=P0.23");
}

bool Button::is_pressed(ButtonId id) {
    if (!is_valid_button(id)) {
        return false;
    }
    return (nrf_gpio_pin_read(BUTTON_PINS[static_cast<uint8_t>(id)]) == 0);
}

bool Button::wait_press(ButtonId id, uint32_t timeout_ms) {
    uint32_t elapsed = 0;

    while (timeout_ms == 0 || elapsed < timeout_ms) {
        if (is_pressed(id)) {
            utkernel::task::sleep_for(DEBOUNCE_MS);
            if (is_pressed(id)) {
                return true;
            }
        }
        utkernel::task::sleep_for(POLL_INTERVAL_MS);
        elapsed += POLL_INTERVAL_MS;
    }
    return false;
}

ButtonId Button::wait_any(uint32_t timeout_ms) {
    uint32_t elapsed = 0;

    while (timeout_ms == 0 || elapsed < timeout_ms) {
        if (is_pressed(ButtonId::A)) {
            utkernel::task::sleep_for(DEBOUNCE_MS);
            if (is_pressed(ButtonId::A)) {
                return ButtonId::A;
            }
        }
        if (is_pressed(ButtonId::B)) {
            utkernel::task::sleep_for(DEBOUNCE_MS);
            if (is_pressed(ButtonId::B)) {
                return ButtonId::B;
            }
        }
        utkernel::task::sleep_for(POLL_INTERVAL_MS);
        elapsed += POLL_INTERVAL_MS;
    }
    return ButtonId::None;
}

}  // namespace drivers
