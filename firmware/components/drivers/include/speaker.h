#pragma once

/**
 * @file speaker.h
 * @brief micro:bit v2.2 スピーカードライバ
 *
 * ハードウェア: JIANGSU HUANENG MLT-8530
 * 接続: P0.00 (PWM 出力)
 * 自己共振周波数: 2700Hz
 */

#include <cstdint>

namespace drivers {

/**
 * @brief スピーカードライバ (PWM)
 */
class Speaker {
  public:
    /** スピーカーを初期化する */
    void init();

    /**
     * 指定周波数のトーンを再生する
     * @param freq_hz  周波数 [Hz], 0 で停止
     */
    void tone(uint32_t freq_hz);

    /** スピーカーを停止する */
    void stop();

    /** スピーカーが再生中か確認する */
    bool is_playing() const;

  private:
    struct InnerState {
        bool playing = false;
    };

    InnerState m_state;
};

}  // namespace drivers
