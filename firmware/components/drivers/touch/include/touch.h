#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * micro:bit v2.2 タッチロゴドライバ
 *
 * ハードウェア:
 *   FACE_TOUCH: P1.04
 *   方式: 静電容量式 (10Mohm プルアップによるRC時定数検出)
 *
 * タッチ時: GNDパッドへの導通でピンがLOWになる
 * 非タッチ時: プルアップによりHIGH
 */

#ifdef __cplusplus
extern "C" {
#endif

/** タッチロゴを初期化する */
void touch_init(void);

/** タッチロゴが現在タッチされているか確認する */
bool touch_is_touched(void);

/** タッチされるまで待つ (タイムアウトms, 0=無限待ち) */
bool touch_wait(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
