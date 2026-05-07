#pragma once

#include <stdint.h>

/**
 * micro:bit v2.2 温度センサドライバ
 *
 * ハードウェア: nRF52833 内蔵温度センサ (TEMP peripheral)
 * レンジ: -40°C ~ 105°C
 * 分解能: 0.25°C
 * 精度: ±5°C (未キャリブレーション)
 */

#ifdef __cplusplus
extern "C" {
#endif

/** 温度センサを初期化する */
void temperature_init(void);

/** 温度を取得する (0.25°C単位の整数値, 例: 100 = 25.0°C) */
int32_t temperature_read_raw(void);

/** 温度を取得する (°C, 整数部のみ) */
int8_t temperature_read(void);

#ifdef __cplusplus
}
#endif
