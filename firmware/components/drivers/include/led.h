#pragma once

/**
 * @file led.h
 * @brief micro:bit v2.2 LED 5x5 マトリクスドライバ
 *
 * LED 配置:
 *   ROW pins: P0.21, P0.22, P0.15, P0.24, P0.19
 *   COL pins: P0.28, P0.11, P0.31, P1.05, P0.30
 *
 * ROW=HIGH, COL=LOW で点灯
 *
 * 使い方:
 *   1. led.init() で GPIO を初期化
 *   2. apps 層でタイマー割り込みを設定し、ISR から led.scan_tick() を周期的に呼ぶ
 *      (推奨: 2ms 周期 → 5行 × 2ms = 10ms/frame = 100Hz)
 *   3. led.set() / led.set_frame() でフレームバッファを更新
 */

#include <cstdint>

namespace drivers {

constexpr int32_t LED_ROWS = 5;
constexpr int32_t LED_COLS = 5;

/**
 * @brief 5x5 LED マトリクスドライバ
 */
class Led {
  public:
    /**
     * LED マトリクスの GPIO を初期化する
     *
     * GPIO ピンの設定とフレームバッファのクリアのみ行う。
     * タイマーの設定は行わない。apps 層でタイマーを設定し、
     * ISR から scan_tick() を呼ぶこと。
     */
    void init();

    /**
     * 1行分のスキャンを実行する (タイマー ISR から呼ぶ)
     *
     * 現在行を消灯 → 次の行に進む → COL 設定 → 行点灯 の順で処理する。
     * apps 層のタイマー割り込みから周期的に呼び出すこと。
     */
    void scan_tick();

    /**
     * 指定位置の LED を点灯/消灯する
     * @param row  行 (0-4)
     * @param col  列 (0-4)
     * @param on   true で点灯
     */
    void set(uint8_t row, uint8_t col, bool on);

    /** すべての LED を消灯する */
    void clear();

    /**
     * 5x5 フレームバッファを一括設定する
     * @param bitmap  bitmap[row] の bit0-4 が col0-4 に対応
     */
    void set_frame(const uint8_t bitmap[LED_ROWS]);

  private:
    /*
     * フレーム内容とスキャン位置は、表示更新とISRの都合で別々に変化する。
     * 論理単位を分けておくことで、バッファ更新と行切り替えの状態を混同しない。
     */
    struct InnerState {
        struct Frame {
            volatile uint8_t buffer[LED_ROWS] = {};
        } frame;

        struct Scan {
            uint8_t current_row = 0;
        } scan;
    };

    InnerState m_state;
};

}  // namespace drivers
