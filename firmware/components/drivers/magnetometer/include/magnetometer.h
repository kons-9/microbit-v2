#pragma once

/**
 * @file magnetometer.h
 * @brief micro:bit v2.2 地磁気センサドライバ
 *
 * ハードウェア: LSM303AGR (ST) — 磁気計部
 * インターフェース: I2C (内部バス)
 *   SCL: P0.08, SDA: P0.16
 *   アドレス: 0x1E
 *
 * レンジ: ±50 gauss (固定)
 * 分解能: 16bit (1.5 mgauss/LSB)
 */

#include <cstdint>

/** 磁場データ (mGauss 単位) */
struct MagnetometerData {
    int16_t m_x;
    int16_t m_y;
    int16_t m_z;
};

/**
 * 地磁気センサを初期化する
 * @return true: 成功, false: WHO_AM_I 不一致等
 */
bool magnetometer_init(void);

/**
 * 磁場データを読み取る
 * @return 磁場データ (mGauss 単位)
 */
MagnetometerData magnetometer_read(void);

/**
 * デバイス ID を確認する
 * @return WHO_AM_I 値 (期待値: 0x40)
 */
uint8_t magnetometer_who_am_i(void);

/**
 * 方位角を取得する
 * @return 方位角 (度, 0-359, 北=0)
 */
uint16_t magnetometer_heading(void);
