#pragma once

/**
 * @file mic.h
 * @brief micro:bit v2.2 マイクドライバ
 *
 * ハードウェア: Knowles SPU0410LR5H-QB-7 MEMS
 * 接続:
 *   MIC_IN:  P0.05 (ADC 入力)
 *   RUN_MIC: P0.20 (マイク電源制御, HIGH=有効)
 *
 * 感度: -38dB ±3dB @ 94dB SPL
 * 周波数範囲: 100Hz ~ 80kHz
 */

#include <cstdint>

/** マイクを初期化する */
void microphone_init(void);

/** マイクの電源を ON にする */
void microphone_enable(void);

/** マイクの電源を OFF にする */
void microphone_disable(void);

/** マイクが有効か確認する */
bool microphone_is_enabled(void);

/**
 * 現在の ADC 値を読み取る
 * @return ADC 値 (0-1023, 10bit)
 */
uint16_t microphone_read(void);

/**
 * 現在の音量レベルを取得する
 * @return レベル (0-255)
 */
uint8_t microphone_get_level(void);
