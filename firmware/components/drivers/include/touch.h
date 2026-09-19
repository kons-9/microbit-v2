#pragma once

/**
 * @file touch.h
 * @brief micro:bit v2.2 タッチロゴドライバ
 *
 * ハードウェア:
 *   FACE_TOUCH: P1.04
 *   方式: 静電容量式 (10Mohm プルアップによる RC 時定数検出)
 */

#include <cstdint>

namespace drivers {

/**
 * @brief タッチロゴドライバ
 */
class Touch {
  public:
    /** タッチロゴを初期化する */
    void init();

    /** タッチロゴが現在タッチされているか確認する */
    bool is_touched();

    /**
     * タッチされるまで待つ
     * @param timeout_ms  タイムアウト [ms], 0=無限待ち
     * @return true: タッチされた, false: タイムアウト
     */
    bool wait(uint32_t timeout_ms);
};

}  // namespace drivers
