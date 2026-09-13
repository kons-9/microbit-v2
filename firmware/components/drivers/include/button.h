#pragma once

/**
 * @file button.h
 * @brief micro:bit v2.2 ボタンドライバ
 *
 * ハードウェア:
 *   Button A: P0.14 (active low, 外部 4.7K プルアップ)
 *   Button B: P0.23 (active low, 外部 4.7K プルアップ)
 */

#include <cstdint>

namespace drivers {

enum class ButtonId : uint8_t {
    A = 0,
    B = 1,
    None = 0xFF, /**< タイムアウト等でボタンが選択されなかった */
};

/**
 * @brief ボタンドライバ
 */
class Button {
  public:
    /** ボタンを初期化する */
    void init();

    /**
     * ボタンが現在押されているか確認する
     * @param id  ボタン ID
     */
    bool is_pressed(ButtonId id);

    /**
     * ボタンが押されるまで待つ
     * @param id          ボタン ID
     * @param timeout_ms  タイムアウト [ms], 0=無限待ち
     * @return true: 押された, false: タイムアウト
     */
    bool wait_press(ButtonId id, uint32_t timeout_ms);

    /**
     * どちらかのボタンが押されるまで待つ
     * @param timeout_ms  タイムアウト [ms], 0=無限待ち
     * @return 押されたボタン ID。タイムアウト時は ButtonId::None
     */
    ButtonId wait_any(uint32_t timeout_ms);
};

}  // namespace drivers
