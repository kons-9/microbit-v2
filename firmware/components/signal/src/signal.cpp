/**
 * @file signal.cpp
 * @brief 信号処理ユーティリティ実装
 */

#include "signal_proc.h"

/* ==================================================================
 * EMA
 * ================================================================== */

void signal_ema_init(SignalEma *ema, float alpha) {
    ema->m_alpha = alpha;
    ema->m_value = 0.0f;
    ema->m_primed = 0;
}

float signal_ema_update(SignalEma *ema, float sample) {
    if (!ema->m_primed) {
        ema->m_value = sample;
        ema->m_primed = 1;
    } else {
        ema->m_value = ema->m_alpha * sample + (1.0f - ema->m_alpha) * ema->m_value;
    }
    return ema->m_value;
}

void signal_ema_reset(SignalEma *ema) {
    ema->m_value = 0.0f;
    ema->m_primed = 0;
}

/* ==================================================================
 * RSSI Accumulator
 * ================================================================== */

void signal_rssi_accum_init(SignalRssiAccum *accum, uint8_t capacity) {
    accum->m_count = 0;
    if (capacity > SIGNAL_RSSI_MAX_SAMPLES) {
        capacity = SIGNAL_RSSI_MAX_SAMPLES;
    }
    accum->m_capacity = capacity;
}

int32_t signal_rssi_accum_add(SignalRssiAccum *accum, int8_t rssi) {
    if (accum->m_count < accum->m_capacity) {
        accum->m_samples[accum->m_count] = rssi;
        accum->m_count++;
    }
    return (accum->m_count >= accum->m_capacity) ? 1 : 0;
}

int8_t signal_rssi_accum_average(const SignalRssiAccum *accum) {
    if (accum->m_count == 0) {
        return 0;
    }

    int32_t sum = 0;
    for (uint8_t i = 0; i < accum->m_count; i++) {
        sum += accum->m_samples[i];
    }
    return static_cast<int8_t>(sum / accum->m_count);
}

void signal_rssi_accum_reset(SignalRssiAccum *accum) {
    accum->m_count = 0;
}
