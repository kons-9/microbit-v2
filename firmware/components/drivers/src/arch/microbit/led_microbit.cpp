/**
 * @file led_microbit.cpp
 * @brief micro:bit v2.2 LED 5x5 マトリクス GPIO 多重化実装
 *
 * GPIO ピン設定とフレームバッファ管理を行う。
 * タイマーの選択・設定は apps 層の責務。apps 層のタイマー ISR から
 * scan_tick() を周期的に呼び出すことでスキャンが動作する。
 */

#include "led.h"

#define LOG_TAG "LED"
#include "log.h"

#include "nrf_gpio.h"

using namespace drivers;

/* ==================================================================
 * Pin Definitions
 * ================================================================== */

static const uint32_t ROW_PINS[LED_ROWS] = {
    NRF_GPIO_PIN_MAP(0, 21),
    NRF_GPIO_PIN_MAP(0, 22),
    NRF_GPIO_PIN_MAP(0, 15),
    NRF_GPIO_PIN_MAP(0, 24),
    NRF_GPIO_PIN_MAP(0, 19),
};

static const uint32_t COL_PINS[LED_COLS] = {
    NRF_GPIO_PIN_MAP(0, 28),
    NRF_GPIO_PIN_MAP(0, 11),
    NRF_GPIO_PIN_MAP(0, 31),
    NRF_GPIO_PIN_MAP(1, 5),
    NRF_GPIO_PIN_MAP(0, 30),
};

namespace drivers {

/* ==================================================================
 * Public API
 * ================================================================== */

void Led::scan_tick() {
    nrf_gpio_pin_clear(ROW_PINS[m_current_row]);

    m_current_row = static_cast<uint8_t>((m_current_row + 1) % LED_ROWS);

    for (int32_t c = 0; c < LED_COLS; c++) {
        if (m_framebuf[m_current_row] & (1U << (LED_COLS - 1 - c))) {
            nrf_gpio_pin_clear(COL_PINS[c]);
        } else {
            nrf_gpio_pin_set(COL_PINS[c]);
        }
    }

    nrf_gpio_pin_set(ROW_PINS[m_current_row]);
}

void Led::init() {
    clear();
    for (int32_t i = 0; i < LED_ROWS; i++) {
        scan_tick();
    }
    LOG_D("init: configuring GPIO");
    for (int32_t i = 0; i < LED_ROWS; i++) {
        nrf_gpio_cfg(ROW_PINS[i],
                     NRF_GPIO_PIN_DIR_OUTPUT,
                     NRF_GPIO_PIN_INPUT_DISCONNECT,
                     NRF_GPIO_PIN_NOPULL,
                     NRF_GPIO_PIN_S0H1,
                     NRF_GPIO_PIN_NOSENSE);
        nrf_gpio_pin_clear(ROW_PINS[i]);
    }
    for (int32_t i = 0; i < LED_COLS; i++) {
        nrf_gpio_cfg(COL_PINS[i],
                     NRF_GPIO_PIN_DIR_OUTPUT,
                     NRF_GPIO_PIN_INPUT_DISCONNECT,
                     NRF_GPIO_PIN_NOPULL,
                     NRF_GPIO_PIN_S0H1,
                     NRF_GPIO_PIN_NOSENSE);
        nrf_gpio_pin_set(COL_PINS[i]);
    }
    m_current_row = 0;

    LOG_D("init: GPIO configured (timer setup is apps layer responsibility)");
}

void Led::set(uint8_t row, uint8_t col, bool on) {
    if (row >= LED_ROWS || col >= LED_COLS) {
        return;
    }
    if (on) {
        m_framebuf[row] |= (1U << col);
    } else {
        m_framebuf[row] &= ~(1U << col);
    }
}

void Led::clear() {
    for (int32_t i = 0; i < LED_ROWS; i++) {
        m_framebuf[i] = 0;
    }
}

void Led::set_frame(const uint8_t bitmap[LED_ROWS]) {
    for (int32_t i = 0; i < LED_ROWS; i++) {
        m_framebuf[i] = bitmap[i];
    }
}

} // namespace drivers
