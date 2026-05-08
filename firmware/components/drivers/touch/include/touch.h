#pragma once

/**
 * @file touch.h
 * @brief micro:bit v2.2 タッチロゴドライバ
 *
 * ハードウェア:
 *   FACE_TOUCH: P1.04
 *   方式: 静電容量式 (10Mohm プルアップによる RC 時定数検出)
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** タッチロゴを初期化する */
void touch_init(void);

/** タッチロゴが現在タッチされているか確認する */
bool touch_is_touched(void);

/**
 * タッチされるまで待つ
 * @param timeout_ms  タイムアウト [ms], 0=無限待ち
 * @return true: タッチされた, false: タイムアウト
 */
bool touch_wait(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
