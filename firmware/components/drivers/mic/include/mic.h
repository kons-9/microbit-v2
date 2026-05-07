#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * micro:bit v2.2 マイクドライバ
 *
 * ハードウェア: Knowles SPU0410LR5H-QB-7 MEMS
 * 接続:
 *   MIC_IN:  P0.05 (ADC入力)
 *   RUN_MIC: P0.20 (マイク電源制御, HIGH=有効)
 *
 * 感度: -38dB ±3dB @ 94dB SPL
 * 周波数範囲: 100Hz ~ 80kHz
 */

#ifdef __cplusplus
extern "C" {
#endif

/** マイクを初期化する */
void mic_init(void);

/** マイクの電源をONにする */
void mic_enable(void);

/** マイクの電源をOFFにする */
void mic_disable(void);

/** マイクが有効か確認する */
bool mic_is_enabled(void);

/** 現在のADC値を読み取る (0-1023, 10bit) */
uint16_t mic_read(void);

/** 現在の音量レベルを取得する (0-255) */
uint8_t mic_get_level(void);

#ifdef __cplusplus
}
#endif
