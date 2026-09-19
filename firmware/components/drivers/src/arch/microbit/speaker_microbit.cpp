/**
 * @file speaker_microbit.cpp
 * @brief micro:bit v2.2 スピーカー PWM 実装
 */

#include "speaker.h"
#include "microbit_driver_config.h"

#define LOG_TAG "SPK"
#include "log.h"

#include "nrf_gpio.h"
#include "nrfx_pwm.h"

namespace {

/*
 * PWMのハードウェアハンドルとEasyDMA用バッファは、tone()の呼び出しを
 * またいで保持する必要がある。公開APIの状態ではないため、InnerStateに
 * まとめて実装内部状態であることを明示する。
 */
struct InnerState {
    struct Pwm {
        nrfx_pwm_t instance = NRFX_PWM_INSTANCE(NRF_PWM0);
    } pwm;

    struct Sequence {
        nrf_pwm_values_common_t values[1] = {};
        nrf_pwm_sequence_t sequence = {
            .values = {.p_common = values},
            .length = 1,
            .repeats = 0,
            .end_delay = 0,
        };
    } sequence;
};

static InnerState s_state{};

}  // namespace

namespace drivers {

void Speaker::init() {
    nrfx_pwm_config_t config = NRFX_PWM_DEFAULT_CONFIG(microbit::config::Speaker::Pin,
                                                       NRF_PWM_PIN_NOT_CONNECTED,
                                                       NRF_PWM_PIN_NOT_CONNECTED,
                                                       NRF_PWM_PIN_NOT_CONNECTED);
    config.base_clock = microbit::config::Speaker::Pwm::BaseClock;
    config.count_mode = microbit::config::Speaker::Pwm::CountMode;
    config.load_mode = microbit::config::Speaker::Pwm::LoadMode;

    if (auto err = nrfx_pwm_init(&s_state.pwm.instance, &config, nullptr, nullptr); err != 0) {
        LOG_E("init failed: %d", err);
        return;
    }
    m_state.playing = false;
    LOG_D("init: PWM0, pin=P0.00");
}

void Speaker::tone(uint32_t freq_hz) {
    if (freq_hz == 0) {
        stop();
        return;
    }

    uint16_t top_value = static_cast<uint16_t>(microbit::config::Speaker::Pwm::ClockHz / freq_hz);
    if (top_value < microbit::config::Speaker::Pwm::MinimumCounterTop) {
        top_value = microbit::config::Speaker::Pwm::MinimumCounterTop;
    }

    s_state.sequence.values[0] = top_value / 2;

    nrfx_pwm_stop(&s_state.pwm.instance, false);

    NRF_PWM0->COUNTERTOP = top_value;
    nrfx_pwm_simple_playback(&s_state.pwm.instance, &s_state.sequence.sequence, 1, NRFX_PWM_FLAG_LOOP);
    m_state.playing = true;
    LOG_D("tone: %lu Hz (top=%u)", freq_hz, top_value);
}

void Speaker::stop() {
    nrfx_pwm_stop(&s_state.pwm.instance, false);
    m_state.playing = false;
    LOG_D("stop");
}

bool Speaker::is_playing() const {
    return m_state.playing;
}

}  // namespace drivers
