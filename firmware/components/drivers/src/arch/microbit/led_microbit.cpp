/**
 * @file led_microbit.cpp
 * @brief micro:bit v2.2 LED 5x5 マトリクス GPIO 多重化実装
 *
 * GPIO ピン設定とフレームバッファ管理を行う。
 * タイマーの選択・設定は apps 層の責務。apps 層のタイマー ISR から
 * scan_tick() を周期的に呼び出すことでスキャンが動作する。
 */

#include "led.h"
#include "microbit_driver_config.h"

#define LOG_TAG "LED"
#include "log.h"

#include "nrf_gpio.h"

using namespace drivers;

namespace drivers {

/* ==================================================================
 * Public API
 * ================================================================== */

void Led::scan_tick() {
    nrf_gpio_pin_clear(microbit::config::Led::RowPins[m_state.scan.current_row]);

    m_state.scan.current_row = static_cast<uint8_t>((m_state.scan.current_row + 1) % LED_ROWS);

    for (int32_t c = 0; c < LED_COLS; c++) {
        if (m_state.frame.buffer[m_state.scan.current_row] & (1U << (LED_COLS - 1 - c))) {
            nrf_gpio_pin_clear(microbit::config::Led::ColPins[c]);
        } else {
            nrf_gpio_pin_set(microbit::config::Led::ColPins[c]);
        }
    }

    nrf_gpio_pin_set(microbit::config::Led::RowPins[m_state.scan.current_row]);
}

void Led::init() {
    clear();
    for (int32_t i = 0; i < LED_ROWS; i++) {
        scan_tick();
    }
    LOG_D("init: configuring GPIO");
    for (int32_t i = 0; i < LED_ROWS; i++) {
        nrf_gpio_cfg(microbit::config::Led::RowPins[i],
                     NRF_GPIO_PIN_DIR_OUTPUT,
                     NRF_GPIO_PIN_INPUT_DISCONNECT,
                     NRF_GPIO_PIN_NOPULL,
                     NRF_GPIO_PIN_S0H1,
                     NRF_GPIO_PIN_NOSENSE);
        nrf_gpio_pin_clear(microbit::config::Led::RowPins[i]);
    }
    for (int32_t i = 0; i < LED_COLS; i++) {
        nrf_gpio_cfg(microbit::config::Led::ColPins[i],
                     NRF_GPIO_PIN_DIR_OUTPUT,
                     NRF_GPIO_PIN_INPUT_DISCONNECT,
                     NRF_GPIO_PIN_NOPULL,
                     NRF_GPIO_PIN_S0H1,
                     NRF_GPIO_PIN_NOSENSE);
        nrf_gpio_pin_set(microbit::config::Led::ColPins[i]);
    }
    m_state.scan.current_row = 0;

    LOG_D("init: GPIO configured (timer setup is apps layer responsibility)");
}

void Led::set(uint8_t row, uint8_t col, bool on) {
    if (row >= LED_ROWS || col >= LED_COLS) {
        return;
    }
    if (on) {
        m_state.frame.buffer[row] |= (1U << col);
    } else {
        m_state.frame.buffer[row] &= ~(1U << col);
    }
}

void Led::clear() {
    for (int32_t i = 0; i < LED_ROWS; i++) {
        m_state.frame.buffer[i] = 0;
    }
}

void Led::set_frame(const uint8_t bitmap[LED_ROWS]) {
    for (int32_t i = 0; i < LED_ROWS; i++) {
        m_state.frame.buffer[i] = bitmap[i];
    }
}

}  // namespace drivers
