#pragma once

/**
 * @file accelerometer.h
 * @brief micro:bit v2.2 加速度センサドライバ
 *
 * ハードウェア: LSM303AGR (ST)
 * インターフェース: I2C (内部バス)
 *   SCL: P0.08, SDA: P0.16
 *   加速度計アドレス: 0x19
 *   割り込み: COMBINED_SENSOR_INT P0.25
 *
 * レンジ: ±2g / ±4g / ±8g / ±16g
 * 分解能: 12bit (high-resolution mode)
 */

#include <cstdint>

/** 加速度レンジ設定 */
enum class AccelerometerRange : uint8_t {
    G2 = 0,
    G4 = 1,
    G8 = 2,
    G16 = 3,
};

/** 加速度データ (mg 単位) */
struct AccelerometerData {
    int16_t m_x;
    int16_t m_y;
    int16_t m_z;
};

/**
 * 加速度センサを初期化する
 * @return true: 成功, false: WHO_AM_I 不一致等
 */
bool accelerometer_init(void);

/**
 * レンジを設定する
 * @param range  加速度レンジ
 */
void accelerometer_set_range(uint8_t range);

/**
 * 加速度データを読み取る
 * @return 加速度データ (mg 単位)
 */
AccelerometerData accelerometer_read(void);

/**
 * デバイス ID を確認する
 * @return WHO_AM_I 値 (期待値: 0x33)
 */
uint8_t accelerometer_who_am_i(void);
