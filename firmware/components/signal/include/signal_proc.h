#pragma once

/**
 * @file signal_proc.h
 * @brief 信号処理ユーティリティ
 *
 * BLE RSSI のスムージング・集約処理を提供する。
 * プラットフォーム非依存。
 */

#include <cstdint>

namespace signal {

/* ==================================================================
 * EMA (Exponential Moving Average) フィルタ
 * ================================================================== */

class Ema {
  public:
    /**
     * @param alpha  平滑化係数 (0.0 ~ 1.0, 大きいほど追従が速い)
     */
    explicit Ema(float alpha);

    /**
     * フィルタを更新する
     * @param sample 新しいサンプル値
     * @return フィルタ出力値
     */
    float update(float sample);

    /** フィルタをリセットする (未初期化状態に戻す) */
    void reset();

    /** 現在のフィルタ出力値 */
    float value() const {
        return m_value;
    }

    /** 初期値セット済みか */
    bool primed() const {
        return m_primed;
    }

  private:
    float m_alpha;
    float m_value = 0.0f;
    bool m_primed = false;
};

/* ==================================================================
 * RSSI 集約
 * ================================================================== */

static constexpr uint8_t RSSI_MAX_SAMPLES = 16;

class RssiAccum {
  public:
    /**
     * @param capacity  集約するサンプル数 (1 ~ RSSI_MAX_SAMPLES)
     */
    explicit RssiAccum(uint8_t capacity);

    /**
     * RSSI サンプルを追加する
     * @param rssi  RSSI 値 (dBm)
     * @return true: capacity に達した
     */
    bool add(int8_t rssi);

    /**
     * 集約済み RSSI 平均値を取得する
     * @return 平均 RSSI (dBm), サンプル数 0 の場合は 0
     */
    int8_t average() const;

    /** 集約器をリセットする */
    void reset();

    /** 現在のサンプル数 */
    uint8_t count() const {
        return m_count;
    }

    /** 要求サンプル数 */
    uint8_t capacity() const {
        return m_capacity;
    }

    /** capacity に達したか */
    bool full() const {
        return m_count >= m_capacity;
    }

  private:
    int8_t m_samples[RSSI_MAX_SAMPLES]{};
    uint8_t m_count = 0;
    uint8_t m_capacity;
};

}  // namespace signal
