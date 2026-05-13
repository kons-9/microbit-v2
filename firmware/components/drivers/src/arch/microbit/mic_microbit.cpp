/**
 * @file mic_microbit.cpp
 * @brief micro:bit v2.2 マイク SAADC 実装
 *
 * nRF52833 では SAADC に simple_mode_set するだけで AIN ピン (P0.28=AIN4 等) の
 * GPIO 出力が干渉される。LED COL1 が P0.28 を使用するため、SAADC は読み取り時のみ
 * 一時的に設定し、完了後に uninit する。
 */

#include "mic.h"

#define LOG_TAG "MIC"
#include "log.h"

#include "nrf_gpio.h"
#include "nrfx_saadc.h"

static constexpr uint32_t MIC_IN_PIN = NRF_GPIO_PIN_MAP(0, 5);
static constexpr uint32_t RUN_MIC_PIN = NRF_GPIO_PIN_MAP(0, 20);

static nrf_saadc_value_t s_sample_buffer[1];

static nrf_saadc_value_t saadc_sample_once() {
    if (auto err = nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY); err != 0) {
        LOG_E("saadc_init failed: %d", err);
        return 0;
    }

    nrfx_saadc_channel_t channel = NRFX_SAADC_DEFAULT_CHANNEL_SE(NRF_SAADC_INPUT_AIN3, 0);
    channel.channel_config.gain = NRF_SAADC_GAIN1_4;
    channel.channel_config.reference = NRF_SAADC_REFERENCE_VDD4;
    channel.channel_config.acq_time = NRF_SAADC_ACQTIME_10US;

    if (auto err = nrfx_saadc_channel_config(&channel); err != 0) {
        LOG_E("saadc_channel_config failed: %d", err);
        nrfx_saadc_uninit();
        return 0;
    }
    if (auto err
        = nrfx_saadc_simple_mode_set((1U << 0), NRF_SAADC_RESOLUTION_10BIT, NRF_SAADC_OVERSAMPLE_DISABLED, nullptr);
        err != 0) {
        LOG_E("saadc_simple_mode_set failed: %d", err);
        nrfx_saadc_uninit();
        return 0;
    }
    if (auto err = nrfx_saadc_buffer_set(s_sample_buffer, 1); err != 0) {
        LOG_E("saadc_buffer_set failed: %d", err);
        nrfx_saadc_uninit();
        return 0;
    }

    if (auto err = nrfx_saadc_mode_trigger(); err != 0) {
        LOG_E("saadc_mode_trigger failed: %d", err);
        nrfx_saadc_uninit();
        return 0;
    }

    nrf_saadc_value_t result = s_sample_buffer[0];

    nrfx_saadc_uninit();

    return result;
}

namespace drivers {

void Microphone::init() {
    nrf_gpio_cfg_output(RUN_MIC_PIN);
    nrf_gpio_pin_clear(RUN_MIC_PIN);
    m_enabled = false;
    LOG_D("init: RUN_MIC=P0.20, MIC_IN=P0.05(AIN3)");
}

void Microphone::enable() {
    nrf_gpio_pin_set(RUN_MIC_PIN);
    m_enabled = true;
    LOG_D("enabled");
}

void Microphone::disable() {
    nrf_gpio_pin_clear(RUN_MIC_PIN);
    m_enabled = false;
    LOG_D("disabled");
}

bool Microphone::is_enabled() const {
    return m_enabled;
}

uint16_t Microphone::read() {
    if (!m_enabled) {
        return 0;
    }

    auto sample = saadc_sample_once();

    if (sample < 0) {
        sample = 0;
    }
    if (sample > 1023) {
        sample = 1023;
    }

    return static_cast<uint16_t>(sample);
}

uint8_t Microphone::get_level() {
    return static_cast<uint8_t>(read() >> 2);
}

} // namespace drivers
