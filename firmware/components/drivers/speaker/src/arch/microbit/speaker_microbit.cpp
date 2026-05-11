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

static nrfx_pwm_t s_pwmInstance = NRFX_PWM_INSTANCE(0);
static bool s_playing = false;

static nrf_pwm_values_common_t s_pwmSeqValues[1];
static nrf_pwm_sequence_t s_pwmSequence = {
    .values = {.p_common = s_pwmSeqValues},
    .length = 1,
    .repeats = 0,
    .end_delay = 0,
};

void speaker_init(void) {
    nrfx_pwm_config_t config = NRFX_PWM_DEFAULT_CONFIG(SPEAKER_PIN,
                                                       NRF_PWM_PIN_NOT_CONNECTED,
                                                       NRF_PWM_PIN_NOT_CONNECTED,
                                                       NRF_PWM_PIN_NOT_CONNECTED);
    config.base_clock = NRF_PWM_CLK_1MHz;
    config.count_mode = NRF_PWM_MODE_UP;
    config.load_mode = NRF_PWM_LOAD_COMMON;

    nrfx_pwm_init(&s_pwmInstance, &config, nullptr, nullptr);
    s_playing = false;
    LOG_D("init: PWM0, pin=P0.00");
}

void speaker_tone(uint32_t freq_hz) {
    if (freq_hz == 0) {
        speaker_stop();
        return;
    }

    uint16_t topValue = static_cast<uint16_t>(1000000U / freq_hz);
    if (topValue < 2) {
        topValue = 2;
    }

    s_pwmSeqValues[0] = topValue / 2;

    nrfx_pwm_stop(&s_pwmInstance, false);

    NRF_PWM0->COUNTERTOP = topValue;
    nrfx_pwm_simple_playback(&s_pwmInstance, &s_pwmSequence, 1, NRFX_PWM_FLAG_LOOP);
    s_playing = true;
    LOG_D("tone: %u Hz (top=%u)", freq_hz, topValue);
}

void speaker_stop(void) {
    nrfx_pwm_stop(&s_pwmInstance, false);
    s_playing = false;
    LOG_D("stop");
}

bool speaker_is_playing(void) {
    return s_playing;
}
