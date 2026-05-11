/**
 * @file led_microbit.cpp
 * @brief micro:bit v2.2 LED 5x5 マトリクス GPIO 多重化実装
 *
 * TIMER2 レジスタ直接操作 + µT-Kernel tk_def_int で自動スキャンする。
 * µT-Kernel が VTOR を独自ベクターテーブル(exchdr_tbl)に向けるため、
 * nrfx ドライバではなく tk_def_int で割り込みハンドラを登録する。
 * apps/ 層からタイマーを供給する必要はない。
 */

#include "led.h"

#define LOG_TAG "LED"
#include "log.h"

#include "nrf.h"
#include "nrf_gpio.h"

#include <tk/tkernel.h>
#include <tk/syslib.h>

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

static uint8_t s_framebuf[LED_ROWS] = {0};
static uint8_t s_currentRow = 0;

/* ==================================================================
 * Timer (TIMER2 register direct access + µT-Kernel interrupt)
 * ================================================================== */

static constexpr uint32_t SCAN_INTERVAL_US = 2000;  // 2ms per row → 10ms/frame = 100Hz
static constexpr uint32_t TIMER2_IRQ_PRIORITY = 7;

static void scan_tick_isr(UINT intno) {
    (void)intno;

    if (NRF_TIMER2->EVENTS_COMPARE[0] == 0) {
        return;
    }
    NRF_TIMER2->EVENTS_COMPARE[0] = 0;

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
    LOG_D("init: configuring GPIO");
    for (int32_t i = 0; i < LED_ROWS; i++) {
        nrf_gpio_cfg(s_rowPins[i],
                     NRF_GPIO_PIN_DIR_OUTPUT,
                     NRF_GPIO_PIN_INPUT_DISCONNECT,
                     NRF_GPIO_PIN_NOPULL,
                     NRF_GPIO_PIN_H0H1,
                     NRF_GPIO_PIN_NOSENSE);
        nrf_gpio_pin_clear(s_rowPins[i]);
    }
    for (int32_t i = 0; i < LED_COLS; i++) {
        nrf_gpio_cfg(s_colPins[i],
                     NRF_GPIO_PIN_DIR_OUTPUT,
                     NRF_GPIO_PIN_INPUT_DISCONNECT,
                     NRF_GPIO_PIN_NOPULL,
                     NRF_GPIO_PIN_H0H1,
                     NRF_GPIO_PIN_NOSENSE);
        nrf_gpio_pin_set(s_colPins[i]);
    }
    s_currentRow = 0;

    /* Register interrupt handler via µT-Kernel */
    T_DINT dint;
    dint.intatr = TA_HLNG;
    dint.inthdr = reinterpret_cast<FP>(scan_tick_isr);
    ER err = tk_def_int(TIMER2_IRQn, &dint);
    if (err < E_OK) {
        LOG_E("tk_def_int failed: %d", static_cast<int>(err));
        return;
    }

    /* Configure TIMER2: 1 MHz timer, 32-bit, cyclic compare on CC[0] */
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;
    NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer;
    NRF_TIMER2->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
    NRF_TIMER2->PRESCALER = 4; /* 16 MHz / 2^4 = 1 MHz */
    NRF_TIMER2->CC[0] = SCAN_INTERVAL_US;
    NRF_TIMER2->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk;
    NRF_TIMER2->INTENSET = TIMER_INTENSET_COMPARE0_Msk;

    EnableInt(TIMER2_IRQn, TIMER2_IRQ_PRIORITY);

    NRF_TIMER2->TASKS_START = 1;
    LOG_D("init: TIMER2 started (%u us/row)", SCAN_INTERVAL_US);
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
