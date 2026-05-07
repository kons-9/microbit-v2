#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * micro:bit v2.2 スピーカードライバ
 *
 * ハードウェア: JIANGSU HUANENG MLT-8530
 * 接続: P0.00 (PWM出力)
 * 自己共振周波数: 2700Hz
 */

#ifdef __cplusplus
extern "C" {
#endif

/** スピーカーを初期化する */
void speaker_init(void);

/** 指定周波数のトーンを再生する (freq_hz: 周波数Hz, 0で停止) */
void speaker_tone(uint32_t freq_hz);

/** スピーカーを停止する */
void speaker_stop(void);

/** スピーカーが再生中か確認する */
bool speaker_is_playing(void);

#ifdef __cplusplus
}
#endif
