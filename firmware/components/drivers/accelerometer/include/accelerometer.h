#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * micro:bit v2.2 加速度センサドライバ
 *
 * ハードウェア: LSM303AGR (ST)
 * インターフェース: I2C (内部バス)
 *   SCL: P0.08 (I2C_INT_SCL)
 *   SDA: P0.16 (I2C_INT_SDA)
 *   加速度計アドレス: 0x19
 *   地磁気計アドレス: 0x1E
 *   割り込み: COMBINED_SENSOR_INT P0.25
 *
 * レンジ: ±2g / ±4g / ±8g / ±16g
 * 分解能: 8/10/12 bits
 */

/** 加速度データ (mg単位) */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} accel_data_t;

/** 加速度レンジ設定 */
typedef enum {
    ACCEL_RANGE_2G = 0,
    ACCEL_RANGE_4G = 1,
    ACCEL_RANGE_8G = 2,
    ACCEL_RANGE_16G = 3,
} accel_range_t;

#ifdef __cplusplus
extern "C" {
#endif

/** 加速度センサを初期化する */
bool accel_init(void);

/** レンジを設定する */
void accel_set_range(accel_range_t range);

/** 加速度データを読み取る (mg単位) */
accel_data_t accel_read(void);

/** デバイスIDを確認する (WHO_AM_I, 期待値: 0x33) */
uint8_t accel_who_am_i(void);

#ifdef __cplusplus
}
#endif
