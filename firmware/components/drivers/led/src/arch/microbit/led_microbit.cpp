#include "led.h"
#include "nrf_gpio.h"

// micro:bit v2.2 LED matrix pin definitions
// ROW pins (active high)
static const uint32_t row_pins[LED_ROWS] = {
    NRF_GPIO_PIN_MAP(0, 21),  // ROW1
    NRF_GPIO_PIN_MAP(0, 22),  // ROW2
    NRF_GPIO_PIN_MAP(0, 15),  // ROW3
    NRF_GPIO_PIN_MAP(0, 24),  // ROW4
    NRF_GPIO_PIN_MAP(0, 19),  // ROW5
};

// COL pins (active low)
static const uint32_t col_pins[LED_COLS] = {
    NRF_GPIO_PIN_MAP(0, 28),  // COL1
    NRF_GPIO_PIN_MAP(0, 11),  // COL2
    NRF_GPIO_PIN_MAP(0, 31),  // COL3
    NRF_GPIO_PIN_MAP(1, 5),   // COL4
    NRF_GPIO_PIN_MAP(0, 30),  // COL5
};

// フレームバッファ: 各行のbit0-4がcol0-4に対応
static uint8_t framebuf[LED_ROWS] = {0};
static uint8_t current_row = 0;

void led_init(void) {
    for (int i = 0; i < LED_ROWS; i++) {
        nrf_gpio_cfg_output(row_pins[i]);
        nrf_gpio_pin_clear(row_pins[i]);
    }
    for (int i = 0; i < LED_COLS; i++) {
        nrf_gpio_cfg_output(col_pins[i]);
        nrf_gpio_pin_set(col_pins[i]);  // COL high = LED off
    }
    current_row = 0;
}

void led_set(uint8_t row, uint8_t col, bool on) {
    if (row >= LED_ROWS || col >= LED_COLS)
        return;
    if (on) {
        framebuf[row] |= (1 << col);
    } else {
        framebuf[row] &= ~(1 << col);
    }
}

void led_clear(void) {
    for (int i = 0; i < LED_ROWS; i++) {
        framebuf[i] = 0;
    }
}

void led_set_frame(const uint8_t bitmap[LED_ROWS]) {
    for (int i = 0; i < LED_ROWS; i++) {
        framebuf[i] = bitmap[i];
    }
}

void led_scan_tick(void) {
    // 前の行を消す
    nrf_gpio_pin_clear(row_pins[current_row]);

    // 次の行へ
    current_row = (current_row + 1) % LED_ROWS;

    // COLピンを設定
    for (int c = 0; c < LED_COLS; c++) {
        if (framebuf[current_row] & (1 << c)) {
            nrf_gpio_pin_clear(col_pins[c]);  // COL low = LED on
        } else {
            nrf_gpio_pin_set(col_pins[c]);  // COL high = LED off
        }
    }

    // 行をアクティブにする
    nrf_gpio_pin_set(row_pins[current_row]);
}
