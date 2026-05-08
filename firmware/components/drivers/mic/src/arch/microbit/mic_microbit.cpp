/**
 * @file mic_microbit.cpp
 * @brief micro:bit v2.2 マイク SAADC 実装
 */

#include "mic.h"

#include "nrf_gpio.h"
#include "nrfx_saadc.h"

static constexpr uint32_t MIC_IN_PIN = NRF_GPIO_PIN_MAP(0, 5);
static constexpr uint32_t RUN_MIC_PIN = NRF_GPIO_PIN_MAP(0, 20);

static bool s_enabled = false;
static nrf_saadc_value_t s_sampleBuffer[1];

void microphone_init(void) {
    nrf_gpio_cfg_output(RUN_MIC_PIN);
    nrf_gpio_pin_clear(RUN_MIC_PIN);
    s_enabled = false;

    nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY);

    nrfx_saadc_channel_t channel = NRFX_SAADC_DEFAULT_CHANNEL_SE(NRF_SAADC_INPUT_AIN3, 0);
    channel.channel_config.gain = NRF_SAADC_GAIN1_4;
    channel.channel_config.reference = NRF_SAADC_REFERENCE_VDD4;
    channel.channel_config.acq_time = NRF_SAADC_ACQTIME_10US;

    nrfx_saadc_channel_config(&channel);

    nrfx_saadc_simple_mode_set((1U << 0), NRF_SAADC_RESOLUTION_10BIT, NRF_SAADC_OVERSAMPLE_DISABLED, nullptr);
    nrfx_saadc_buffer_set(s_sampleBuffer, 1);
}

void microphone_enable(void) {
    nrf_gpio_pin_set(RUN_MIC_PIN);
    s_enabled = true;
}

void microphone_disable(void) {
    nrf_gpio_pin_clear(RUN_MIC_PIN);
    s_enabled = false;
}

bool microphone_is_enabled(void) {
    return s_enabled;
}

uint16_t microphone_read(void) {
    if (!s_enabled) {
        return 0;
    }

    nrfx_saadc_mode_trigger();

    auto sample = s_sampleBuffer[0];
    nrfx_saadc_buffer_set(s_sampleBuffer, 1);

    if (sample < 0) {
        sample = 0;
    }
    if (sample > 1023) {
        sample = 1023;
    }

    return static_cast<uint16_t>(sample);
}

uint8_t microphone_get_level(void) {
    return static_cast<uint8_t>(microphone_read() >> 2);
}
