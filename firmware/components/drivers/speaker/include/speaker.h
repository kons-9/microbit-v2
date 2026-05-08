#pragma once

/**
 * @file speaker.h
 * @brief micro:bit v2.2 スピーカードライバ
 *
 * ハードウェア: JIANGSU HUANENG MLT-8530
 * 接続: P0.00 (PWM 出力)
 * 自己共振周波数: 2700Hz
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** スピーカーを初期化する */
void speaker_init(void);

/**
 * 指定周波数のトーンを再生する
 * @param freq_hz  周波数 [Hz], 0 で停止
 */
void speaker_tone(uint32_t freq_hz);

/** スピーカーを停止する */
void speaker_stop(void);

/** スピーカーが再生中か確認する */
bool speaker_is_playing(void);

#ifdef __cplusplus
}
#endif
