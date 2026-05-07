#pragma once

#include <stdint.h>

/**
 * micro:bit v2.2 LED 5x5 マトリクスドライバ
 *
 * LED配置:
 *   ROW pins: P0.21, P0.22, P0.15, P0.24, P0.19
 *   COL pins: P0.28, P0.11, P0.31, P1.05, P0.30
 *
 * ROW=HIGH, COL=LOW で点灯
 */

#define LED_ROWS 5
#define LED_COLS 5

#ifdef __cplusplus
extern "C" {
#endif

/** LED マトリクスを初期化する */
void led_init(void);

/** 指定した位置のLEDを点灯/消灯する (row: 0-4, col: 0-4) */
void led_set(uint8_t row, uint8_t col, bool on);

/** すべてのLEDを消灯する */
void led_clear(void);

/** 5x5 フレームバッファを一括設定する (bitmap[row] のbit0-4がcol0-4に対応) */
void led_set_frame(const uint8_t bitmap[LED_ROWS]);

/** マトリクスの1行分を走査する（定期タイマから呼び出す） */
void led_scan_tick(void);

#ifdef __cplusplus
}
#endif
