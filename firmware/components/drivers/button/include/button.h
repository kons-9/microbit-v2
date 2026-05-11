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

enum class ButtonId : uint8_t {
    A = 0,
    B = 1,
};

/** ボタンを初期化する */
void button_init(void);

/**
 * ボタンが現在押されているか確認する
 * @param id  ボタン ID
 */
bool button_is_pressed(uint8_t id);

/**
 * ボタンが押されるまで待つ
 * @param id          ボタン ID
 * @param timeout_ms  タイムアウト [ms], 0=無限待ち
 * @return true: 押された, false: タイムアウト
 */
bool button_wait_press(uint8_t id, uint32_t timeout_ms);

/**
 * どちらかのボタンが押されるまで待つ
 * @param timeout_ms  タイムアウト [ms], 0=無限待ち
 * @return 押されたボタン ID
 */
uint8_t button_wait_any(uint32_t timeout_ms);
