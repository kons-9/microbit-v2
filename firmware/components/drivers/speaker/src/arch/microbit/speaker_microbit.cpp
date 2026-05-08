#include "speaker.h"
#include "nrf_gpio.h"
#include "nrfx_pwm.h"

// micro:bit v2.2 speaker pin
#define SPEAKER_PIN NRF_GPIO_PIN_MAP(0, 0)

static nrfx_pwm_t pwm_instance = NRFX_PWM_INSTANCE(0);
static bool playing = false;

// PWMシーケンス用バッファ
static nrf_pwm_values_common_t pwm_seq_values[1];
static nrf_pwm_sequence_t pwm_sequence = {
    .values = {.p_common = pwm_seq_values},
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

    nrfx_pwm_init(&pwm_instance, &config, NULL, NULL);
    playing = false;
}

void speaker_tone(uint32_t freq_hz) {
    if (freq_hz == 0) {
        speaker_stop();
        return;
    }

    // PWM周期 = 1MHz / freq_hz
    uint16_t top_value = 1000000 / freq_hz;
    if (top_value < 2)
        top_value = 2;

    // duty 50%
    pwm_seq_values[0] = top_value / 2;

    nrfx_pwm_stop(&pwm_instance, false);

    // top_valueを設定してシーケンスを再生
    NRF_PWM0->COUNTERTOP = top_value;
    nrfx_pwm_simple_playback(&pwm_instance, &pwm_sequence, 1, NRFX_PWM_FLAG_LOOP);
    playing = true;
}

void speaker_stop(void) {
    nrfx_pwm_stop(&pwm_instance, false);
    playing = false;
}

bool speaker_is_playing(void) {
    return playing;
}
