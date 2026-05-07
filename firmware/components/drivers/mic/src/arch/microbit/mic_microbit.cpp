#include "mic.h"
#include "nrf_gpio.h"
#include "nrfx_saadc.h"

// micro:bit v2.2 microphone pins
#define MIC_IN_PIN NRF_GPIO_PIN_MAP(0, 5)    // ADC input (AIN3)
#define RUN_MIC_PIN NRF_GPIO_PIN_MAP(0, 20)  // Power control

static bool enabled = false;
static nrf_saadc_value_t sample_buffer[1];

void mic_init(void) {
    // RUN_MIC pinを出力に設定（初期: OFF）
    nrf_gpio_cfg_output(RUN_MIC_PIN);
    nrf_gpio_pin_clear(RUN_MIC_PIN);
    enabled = false;

    // SAADC 初期化
    nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY);

    // チャンネル設定 (AIN3 = P0.05)
    nrfx_saadc_channel_t channel = NRFX_SAADC_DEFAULT_CHANNEL_SE(NRF_SAADC_INPUT_AIN3, 0);
    channel.channel_config.gain = NRF_SAADC_GAIN1_4;
    channel.channel_config.reference = NRF_SAADC_REFERENCE_VDD4;
    channel.channel_config.acq_time = NRF_SAADC_ACQTIME_10US;

    nrfx_saadc_channel_config(&channel);

    // ブロッキングモード (event_handler = NULL)
    nrfx_saadc_simple_mode_set((1U << 0), NRF_SAADC_RESOLUTION_10BIT, NRF_SAADC_OVERSAMPLE_DISABLED, NULL);
    nrfx_saadc_buffer_set(sample_buffer, 1);
}

void mic_enable(void) {
    nrf_gpio_pin_set(RUN_MIC_PIN);
    enabled = true;
}

void mic_disable(void) {
    nrf_gpio_pin_clear(RUN_MIC_PIN);
    enabled = false;
}

bool mic_is_enabled(void) {
    return enabled;
}

uint16_t mic_read(void) {
    if (!enabled)
        return 0;

    nrfx_saadc_mode_trigger();

    nrf_saadc_value_t sample = sample_buffer[0];

    // 次回の変換用にバッファを再設定
    nrfx_saadc_buffer_set(sample_buffer, 1);

    // クランプ (負の値を0に)
    if (sample < 0)
        sample = 0;
    if (sample > 1023)
        sample = 1023;

    return (uint16_t)sample;
}

uint8_t mic_get_level(void) {
    uint16_t raw = mic_read();
    // 10bitを8bitにスケーリング
    return (uint8_t)(raw >> 2);
}