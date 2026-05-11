#pragma once

/**
 * @file led.h
 * @brief micro:bit v2.2 LED 5x5 マトリクスドライバ
 *
 * LED 配置:
 *   ROW pins: P0.21, P0.22, P0.15, P0.24, P0.19
 *   COL pins: P0.28, P0.11, P0.31, P1.05, P0.30
 *
 * ROW=HIGH, COL=LOW で点灯
 */

#include <cstdint>

constexpr int32_t LED_ROWS = 5;
constexpr int32_t LED_COLS = 5;

/** LED マトリクスを初期化する */
void led_init(void);

/**
 * 指定位置の LED を点灯/消灯する
 * @param row  行 (0-4)
 * @param col  列 (0-4)
 * @param on   true で点灯
 */
void led_set(uint8_t row, uint8_t col, bool on);

/** すべての LED を消灯する */
void led_clear(void);

/**
 * 5x5 フレームバッファを一括設定する
 * @param bitmap  bitmap[row] の bit0-4 が col0-4 に対応
 */
void led_set_frame(const uint8_t bitmap[LED_ROWS]);
