/**
 * @file led_microbit.cpp
 * @brief micro:bit v2.2 LED 5x5 マトリクス GPIO 多重化実装
 *
 * GPIO ピン設定とフレームバッファ管理を行う。
 * タイマーの選択・設定は apps 層の責務。apps 層のタイマー ISR から
 * led_scan_tick() を周期的に呼び出すことでスキャンが動作する。
 */

#include "led.h"

#define LOG_TAG "LED"
#include "log.h"

#include "nrf_gpio.h"

/* ==================================================================
 * Pin Definitions
 * ================================================================== */

static const uint32_t s_rowPins[LED_ROWS] = {
    NRF_GPIO_PIN_MAP(0, 21),
    NRF_GPIO_PIN_MAP(0, 22),
    NRF_GPIO_PIN_MAP(0, 15),
    NRF_GPIO_PIN_MAP(0, 24),
    NRF_GPIO_PIN_MAP(0, 19),
};

static const uint32_t s_colPins[LED_COLS] = {
    NRF_GPIO_PIN_MAP(0, 28),
    NRF_GPIO_PIN_MAP(0, 11),
    NRF_GPIO_PIN_MAP(0, 31),
    NRF_GPIO_PIN_MAP(1, 5),
    NRF_GPIO_PIN_MAP(0, 30),
};

/* ==================================================================
 * State
 * ================================================================== */

static volatile uint8_t s_framebuf[LED_ROWS] = {0};
static uint8_t s_currentRow = 0;

/* ==================================================================
 * Scan logic (called from apps layer timer ISR)
 * ================================================================== */

void led_scan_tick(void) {
    nrf_gpio_pin_clear(s_rowPins[s_currentRow]);

    s_currentRow = static_cast<uint8_t>((s_currentRow + 1) % LED_ROWS);

    for (int32_t c = 0; c < LED_COLS; c++) {
        if (s_framebuf[s_currentRow] & (1U << c)) {
            nrf_gpio_pin_clear(s_colPins[c]);
        } else {
            nrf_gpio_pin_set(s_colPins[c]);
        }
    }

    nrf_gpio_pin_set(s_rowPins[s_currentRow]);
}

/* ==================================================================
 * API
 * ================================================================== */

void led_init(void) {
    led_clear();
    for (int32_t i = 0; i < LED_ROWS; i++) {
        led_scan_tick();
    }
    LOG_D("init: configuring GPIO");
    for (int32_t i = 0; i < LED_ROWS; i++) {
        nrf_gpio_cfg(s_rowPins[i],
                     NRF_GPIO_PIN_DIR_OUTPUT,
                     NRF_GPIO_PIN_INPUT_DISCONNECT,
                     NRF_GPIO_PIN_NOPULL,
                     NRF_GPIO_PIN_S0H1,
                     NRF_GPIO_PIN_NOSENSE);
        nrf_gpio_pin_clear(s_rowPins[i]);
    }
    for (int32_t i = 0; i < LED_COLS; i++) {
        nrf_gpio_cfg(s_colPins[i],
                     NRF_GPIO_PIN_DIR_OUTPUT,
                     NRF_GPIO_PIN_INPUT_DISCONNECT,
                     NRF_GPIO_PIN_NOPULL,
                     NRF_GPIO_PIN_S0H1,
                     NRF_GPIO_PIN_NOSENSE);
        nrf_gpio_pin_set(s_colPins[i]);
    }
    s_currentRow = 0;

    LOG_D("init: GPIO configured (timer setup is apps layer responsibility)");
}

void led_set(uint8_t row, uint8_t col, bool on) {
    if (row >= LED_ROWS || col >= LED_COLS) {
        return;
    }
    if (on) {
        s_framebuf[row] |= (1U << col);
    } else {
        s_framebuf[row] &= ~(1U << col);
    }
}

void led_clear(void) {
    for (int32_t i = 0; i < LED_ROWS; i++) {
        s_framebuf[i] = 0;
    }
}

void led_set_frame(const uint8_t bitmap[LED_ROWS]) {
    for (int32_t i = 0; i < LED_ROWS; i++) {
        s_framebuf[i] = bitmap[i];
    }
}
