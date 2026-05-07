#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * micro:bit v2.2 地磁気センサドライバ
 *
 * ハードウェア: LSM303AGR (ST) - 磁気計部
 * インターフェース: I2C (内部バス)
 *   SCL: P0.08 (I2C_INT_SCL)
 *   SDA: P0.16 (I2C_INT_SDA)
 *   アドレス: 0x1E
 *   割り込み: COMBINED_SENSOR_INT P0.25
 *
 * レンジ: ±50 gauss (固定)
 * 分解能: 16bit (1.5 mgauss/LSB)
 */

/** 磁場データ (mGauss単位) */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} mag_data_t;

#ifdef __cplusplus
extern "C" {
#endif

/** 地磁気センサを初期化する */
bool mag_init(void);

/** 磁場データを読み取る (mGauss単位) */
mag_data_t mag_read(void);

/** デバイスIDを確認する (WHO_AM_I, 期待値: 0x40) */
uint8_t mag_who_am_i(void);

/** 方位角を取得する (度, 0-359, 北=0) */
uint16_t mag_heading(void);

#ifdef __cplusplus
}
#endif
