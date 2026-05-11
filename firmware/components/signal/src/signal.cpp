/**
 * @file signal.cpp
 * @brief 信号処理ユーティリティ実装
 */

#include "signal_proc.h"

namespace signal {

/* ==================================================================
 * Ema
 * ================================================================== */

Ema::Ema(float alpha)
    : m_alpha(alpha) {
}

float Ema::update(float sample) {
    if (!m_primed) {
        m_value = sample;
        m_primed = true;
    } else {
        m_value = m_alpha * sample + (1.0f - m_alpha) * m_value;
    }
    return m_value;
}

void Ema::reset() {
    m_value = 0.0f;
    m_primed = false;
}

/* ==================================================================
 * RssiAccum
 * ================================================================== */

RssiAccum::RssiAccum(uint8_t capacity)
    : m_capacity(capacity > RSSI_MAX_SAMPLES ? RSSI_MAX_SAMPLES : capacity) {
}

bool RssiAccum::add(int8_t rssi) {
    if (m_count < m_capacity) {
        m_samples[m_count] = rssi;
        m_count++;
    }
    return m_count >= m_capacity;
}

int8_t RssiAccum::average() const {
    if (m_count == 0) {
        return 0;
    }

    int32_t sum = 0;
    for (uint8_t i = 0; i < m_count; i++) {
        sum += m_samples[i];
    }
    return static_cast<int8_t>(sum / m_count);
}

void RssiAccum::reset() {
    m_count = 0;
}

}  // namespace signal
