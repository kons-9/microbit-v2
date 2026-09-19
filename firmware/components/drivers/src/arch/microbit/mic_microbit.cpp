/**
 * @file mic_microbit.cpp
 * @brief micro:bit v2.2 マイク SAADC 実装
 *
 * nRF52833 では SAADC に simple_mode_set するだけで AIN ピン (P0.28=AIN4 等) の
 * GPIO 出力が干渉される。LED COL1 が P0.28 を使用するため、SAADC は読み取り時のみ
 * 一時的に設定し、完了後に uninit する。
 */

#include "mic.h"
#include "microbit_driver_config.h"

#define LOG_TAG "MIC"
#include "log.h"

#include "nrf_gpio.h"
#include "nrfx_saadc.h"

namespace {

/*
 * SAADCが変換中に参照するEasyDMAバッファは、変換完了まで生存して
 * いなければならない。公開APIの状態ではないためInnerStateにまとめる。
 */
struct InnerState {
    struct Adc {
        nrf_saadc_value_t sample_buffer[1] = {};
    } adc;
};

static InnerState s_state{};

}  // namespace

static nrf_saadc_value_t saadc_sample_once() {
    if (auto err = nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY); err != 0) {
        LOG_E("saadc_init failed: %d", err);
        return 0;
    }

    nrfx_saadc_channel_t channel
        = NRFX_SAADC_DEFAULT_CHANNEL_SE(drivers::microbit::config::Microphone::Adc::Input,
                                        drivers::microbit::config::Microphone::Adc::Channel);
    channel.channel_config.gain = drivers::microbit::config::Microphone::Adc::Gain;
    channel.channel_config.reference = drivers::microbit::config::Microphone::Adc::Reference;
    channel.channel_config.acq_time = drivers::microbit::config::Microphone::Adc::AcquisitionTime;

    if (auto err = nrfx_saadc_channel_config(&channel); err != 0) {
        LOG_E("saadc_channel_config failed: %d", err);
        nrfx_saadc_uninit();
        return 0;
    }
    if (auto err
        = nrfx_saadc_simple_mode_set(drivers::microbit::config::Microphone::Adc::ChannelMask,
                                      drivers::microbit::config::Microphone::Adc::Resolution,
                                      drivers::microbit::config::Microphone::Adc::Oversample,
                                      nullptr);
        err != 0) {
        LOG_E("saadc_simple_mode_set failed: %d", err);
        nrfx_saadc_uninit();
        return 0;
    }
    if (auto err = nrfx_saadc_buffer_set(s_state.adc.sample_buffer, 1); err != 0) {
        LOG_E("saadc_buffer_set failed: %d", err);
        nrfx_saadc_uninit();
        return 0;
    }

    if (auto err = nrfx_saadc_mode_trigger(); err != 0) {
        LOG_E("saadc_mode_trigger failed: %d", err);
        nrfx_saadc_uninit();
        return 0;
    }

    nrf_saadc_value_t result = s_state.adc.sample_buffer[0];

    nrfx_saadc_uninit();

    return result;
}

namespace drivers {

void Microphone::init() {
    nrf_gpio_cfg_output(microbit::config::Microphone::RunPin);
    nrf_gpio_pin_clear(microbit::config::Microphone::RunPin);
    m_state.enabled = false;
    LOG_D("init: RUN_MIC=P0.20, MIC_IN=P0.05(AIN3)");
}

void Microphone::enable() {
    nrf_gpio_pin_set(microbit::config::Microphone::RunPin);
    m_state.enabled = true;
    LOG_D("enabled");
}

void Microphone::disable() {
    nrf_gpio_pin_clear(microbit::config::Microphone::RunPin);
    m_state.enabled = false;
    LOG_D("disabled");
}

bool Microphone::is_enabled() const {
    return m_state.enabled;
}

uint16_t Microphone::read() {
    if (!m_state.enabled) {
        return 0;
    }

    auto sample = saadc_sample_once();

    if (sample < microbit::config::Microphone::Adc::MinimumValue) {
        sample = microbit::config::Microphone::Adc::MinimumValue;
    }
    if (sample > microbit::config::Microphone::Adc::MaximumValue) {
        sample = microbit::config::Microphone::Adc::MaximumValue;
    }

    return static_cast<uint16_t>(sample);
}

uint8_t Microphone::get_level() {
    return static_cast<uint8_t>(read() >> microbit::config::Microphone::Adc::LevelShift);
}

}  // namespace drivers
