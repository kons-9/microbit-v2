#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * micro:bit v2.2 ボタンドライバ
 *
 * ハードウェア:
 *   Button A: P0.14 (active low, external 4.7K pull-up)
 *   Button B: P0.23 (active low, external 4.7K pull-up)
 *
 * 押下時: LOW (0)
 * 解放時: HIGH (1)
 */

typedef enum {
    BUTTON_A = 0,
    BUTTON_B = 1,
} button_id_t;

#ifdef __cplusplus
extern "C" {
#endif

/** ボタンを初期化する */
void button_init(void);

/** ボタンが現在押されているか確認する */
bool button_is_pressed(button_id_t id);

/** ボタンが押されるまで待つ (タイムアウトms, 0=無限待ち) */
bool button_wait_press(button_id_t id, uint32_t timeout_ms);

/** どちらかのボタンが押されるまで待つ。押されたボタンIDを返す */
button_id_t button_wait_any(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
