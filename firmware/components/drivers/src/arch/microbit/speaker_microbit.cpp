/**
 * @file speaker_microbit.cpp
 * @brief micro:bit v2.2 スピーカー PWM 実装
 */

#include "speaker.h"

#define LOG_TAG "SPK"
#include "log.h"

#include "nrf_gpio.h"
#include "nrfx_pwm.h"

static constexpr uint32_t SPEAKER_PIN = NRF_GPIO_PIN_MAP(0, 0);

static nrfx_pwm_t s_pwm_instance = NRFX_PWM_INSTANCE(NRF_PWM0);

static nrf_pwm_values_common_t s_pwm_seq_values[1];
static nrf_pwm_sequence_t s_pwm_sequence = {
    .values = {.p_common = s_pwm_seq_values},
    .length = 1,
    .repeats = 0,
    .end_delay = 0,
};

namespace drivers {

void Speaker::init() {
    nrfx_pwm_config_t config = NRFX_PWM_DEFAULT_CONFIG(SPEAKER_PIN,
                                                       NRF_PWM_PIN_NOT_CONNECTED,
                                                       NRF_PWM_PIN_NOT_CONNECTED,
                                                       NRF_PWM_PIN_NOT_CONNECTED);
    config.base_clock = NRF_PWM_CLK_1MHz;
    config.count_mode = NRF_PWM_MODE_UP;
    config.load_mode = NRF_PWM_LOAD_COMMON;

    if (auto err = nrfx_pwm_init(&s_pwm_instance, &config, nullptr, nullptr); err != 0) {
        LOG_E("init failed: %d", err);
        return;
    }
    m_playing = false;
    LOG_D("init: PWM0, pin=P0.00");
}

void Speaker::tone(uint32_t freq_hz) {
    if (freq_hz == 0) {
        stop();
        return;
    }

    uint16_t top_value = static_cast<uint16_t>(1000000U / freq_hz);
    if (top_value < 2) {
        top_value = 2;
    }

    s_pwm_seq_values[0] = top_value / 2;

    nrfx_pwm_stop(&s_pwm_instance, false);

    NRF_PWM0->COUNTERTOP = top_value;
    nrfx_pwm_simple_playback(&s_pwm_instance, &s_pwm_sequence, 1, NRFX_PWM_FLAG_LOOP);
    m_playing = true;
    LOG_D("tone: %lu Hz (top=%u)", freq_hz, top_value);
}

void Speaker::stop() {
    nrfx_pwm_stop(&s_pwm_instance, false);
    m_playing = false;
    LOG_D("stop");
}

bool Speaker::is_playing() const {
    return m_playing;
}

}  // namespace drivers
