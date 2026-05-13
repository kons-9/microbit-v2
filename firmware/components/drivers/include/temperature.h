#pragma once

/**
 * @file temperature.h
 * @brief micro:bit v2.2 温度センサドライバ
 *
 * ハードウェア: nRF52833 内蔵温度センサ (TEMP peripheral)
 * レンジ: -40°C ~ 105°C
 * 分解能: 0.25°C
 */

#include <cstdint>

namespace drivers {

/**
 * @brief 内蔵温度センサドライバ
 */
class Temperature {
  public:
    /** 温度センサを初期化する */
    void init();

    /**
     * 温度を取得する (0.25°C 単位の整数値)
     * @return 生値 (例: 100 = 25.0°C)
     */
    int32_t read_raw();

    /**
     * 温度を取得する (°C, 整数部のみ)
     * @return 温度 [°C]
     */
    int8_t read();
};

}  // namespace drivers
