#pragma once

/**
 * @file signal_proc.h
 * @brief 信号処理ユーティリティ
 *
 * BLE RSSI のスムージング・集約処理を提供する。
 * プラットフォーム非依存。
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================================================================
 * EMA (Exponential Moving Average) フィルタ
 * ================================================================== */

/** EMA フィルタ状態 */
typedef struct {
    float m_alpha;    /**< 平滑化係数 (0.0 ~ 1.0, 大きいほど追従が速い) */
    float m_value;    /**< 現在のフィルタ出力値 */
    uint8_t m_primed; /**< 初期値セット済みフラグ */
} SignalEma;

/**
 * EMA フィルタを初期化する
 * @param ema    フィルタ状態
 * @param alpha  平滑化係数 (0.0 ~ 1.0)
 * @post ema->m_primed == 0
 */
void signal_ema_init(SignalEma *ema, float alpha);

/**
 * EMA フィルタを更新する
 * @param ema    フィルタ状態
 * @param sample 新しいサンプル値
 * @return フィルタ出力値
 */
float signal_ema_update(SignalEma *ema, float sample);

/**
 * EMA フィルタをリセットする (未初期化状態に戻す)
 * @param ema  フィルタ状態
 */
void signal_ema_reset(SignalEma *ema);

/* ==================================================================
 * RSSI 集約
 * ================================================================== */

/** RSSI サンプルバッファの最大サイズ */
static constexpr uint8_t SIGNAL_RSSI_MAX_SAMPLES = 16;

/** RSSI 集約器 */
typedef struct {
    int8_t m_samples[SIGNAL_RSSI_MAX_SAMPLES]; /**< サンプルバッファ */
    uint8_t m_count;                           /**< 現在のサンプル数 */
    uint8_t m_capacity;                        /**< 要求サンプル数 */
} SignalRssiAccum;

/**
 * RSSI 集約器を初期化する
 * @param accum     集約器
 * @param capacity  集約するサンプル数 (1 ~ SIGNAL_RSSI_MAX_SAMPLES)
 * @post accum->m_count == 0
 */
void signal_rssi_accum_init(SignalRssiAccum *accum, uint8_t capacity);

/**
 * RSSI サンプルを追加する
 * @param accum   集約器
 * @param rssi    RSSI 値 (dBm)
 * @return 1: capacity に達した, 0: まだ
 */
int32_t signal_rssi_accum_add(SignalRssiAccum *accum, int8_t rssi);

/**
 * 集約済み RSSI 平均値を取得する
 * @param accum  集約器
 * @return 平均 RSSI (dBm), サンプル数 0 の場合は 0
 */
int8_t signal_rssi_accum_average(const SignalRssiAccum *accum);

/**
 * 集約器をリセットする
 * @param accum  集約器
 */
void signal_rssi_accum_reset(SignalRssiAccum *accum);

#ifdef __cplusplus
}
#endif
