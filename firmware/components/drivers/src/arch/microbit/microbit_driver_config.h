#pragma once

/**
 * @file microbit_driver_config.h
 * @brief micro:bitドライバの内部ハードウェア設定
 *
 * 公開ドライバAPIには出さない、micro:bit v2.2固有のピン配置、デバイス
 * アドレス、レジスタ、タイミングをまとめる。設定の関係を構造体ごとに
 * 分けておくことで、実行時状態（InnerState）と混同せずに変更できる。
 */

#include "nrf_gpio.h"
#include "nrf_pwm.h"
#include "nrf_saadc.h"
#include "nrf_twim.h"

#include <cstddef>
#include <cstdint>

namespace drivers::microbit::config {

struct Button {
    static constexpr uint32_t PinA = NRF_GPIO_PIN_MAP(0, 14);
    static constexpr uint32_t PinB = NRF_GPIO_PIN_MAP(0, 23);
    static constexpr uint32_t Pins[] = {PinA, PinB};
    static constexpr uint32_t DebounceMs = 50;
    static constexpr uint32_t PollIntervalMs = 10;
};

struct Touch {
    static constexpr uint32_t FacePin = NRF_GPIO_PIN_MAP(1, 4);
    static constexpr uint32_t DebounceMs = 50;
    static constexpr uint32_t PollIntervalMs = 10;
};

struct Led {
    static constexpr uint32_t RowPins[] = {
        NRF_GPIO_PIN_MAP(0, 21),
        NRF_GPIO_PIN_MAP(0, 22),
        NRF_GPIO_PIN_MAP(0, 15),
        NRF_GPIO_PIN_MAP(0, 24),
        NRF_GPIO_PIN_MAP(0, 19),
    };
    static constexpr uint32_t ColPins[] = {
        NRF_GPIO_PIN_MAP(0, 28),
        NRF_GPIO_PIN_MAP(0, 11),
        NRF_GPIO_PIN_MAP(0, 31),
        NRF_GPIO_PIN_MAP(1, 5),
        NRF_GPIO_PIN_MAP(0, 30),
    };
};

struct Speaker {
    static constexpr uint32_t Pin = NRF_GPIO_PIN_MAP(0, 0);

    struct Pwm {
        static constexpr auto BaseClock = NRF_PWM_CLK_1MHz;
        static constexpr auto CountMode = NRF_PWM_MODE_UP;
        static constexpr auto LoadMode = NRF_PWM_LOAD_COMMON;
        static constexpr uint32_t ClockHz = 1000000;
        static constexpr uint16_t MinimumCounterTop = 2;
    };
};

struct Microphone {
    static constexpr uint32_t InputPin = NRF_GPIO_PIN_MAP(0, 5);
    static constexpr uint32_t RunPin = NRF_GPIO_PIN_MAP(0, 20);

    struct Adc {
        static constexpr auto Input = NRF_SAADC_INPUT_AIN3;
        static constexpr uint32_t Channel = 0;
        static constexpr auto Gain = NRF_SAADC_GAIN1_4;
        static constexpr auto Reference = NRF_SAADC_REFERENCE_VDD4;
        static constexpr auto AcquisitionTime = NRF_SAADC_ACQTIME_10US;
        static constexpr auto Resolution = NRF_SAADC_RESOLUTION_10BIT;
        static constexpr auto Oversample = NRF_SAADC_OVERSAMPLE_DISABLED;
        static constexpr uint32_t ChannelMask = 1U << Channel;
        static constexpr int16_t MinimumValue = 0;
        static constexpr int16_t MaximumValue = 1023;
        static constexpr uint8_t LevelShift = 2;
    };
};

struct Uart {
    static constexpr uint32_t TxPin = NRF_GPIO_PIN_MAP(0, 6);
    static constexpr uint32_t RxPin = NRF_GPIO_PIN_MAP(1, 8);
    static constexpr size_t TxChunkSize = 256;
};

struct Accelerometer {
    struct Bus {
        static constexpr uint32_t SclPin = NRF_GPIO_PIN_MAP(0, 8);
        static constexpr uint32_t SdaPin = NRF_GPIO_PIN_MAP(0, 16);
        static constexpr auto Frequency = NRF_TWIM_FREQ_400K;
    };

    struct Device {
        static constexpr uint8_t Address = 0x19;
        static constexpr uint8_t WhoAmI = 0x33;
    };

    struct Register {
        static constexpr uint8_t WhoAmI = 0x0F;
        static constexpr uint8_t CtrlReg1 = 0x20;
        static constexpr uint8_t CtrlReg4 = 0x23;
        static constexpr uint8_t OutXL = 0x28;
    };

    struct Setup {
        static constexpr uint8_t CtrlReg1Value = 0x57;
        static constexpr uint8_t CtrlReg4BaseValue = 0x08;
        static constexpr uint8_t MultiByteReadMask = 0x80;
        static constexpr uint8_t RawValueShift = 4;
    };

    static constexpr int16_t ScaleFactor[] = {1, 2, 4, 12};
};

struct Magnetometer {
    struct Bus {
        static constexpr uint32_t SclPin = NRF_GPIO_PIN_MAP(0, 8);
        static constexpr uint32_t SdaPin = NRF_GPIO_PIN_MAP(0, 16);
        static constexpr auto Frequency = NRF_TWIM_FREQ_400K;
    };

    struct Device {
        static constexpr uint8_t Address = 0x1E;
        static constexpr uint8_t WhoAmI = 0x40;
    };

    struct Register {
        static constexpr uint8_t WhoAmI = 0x4F;
        static constexpr uint8_t ConfigA = 0x60;
        static constexpr uint8_t ConfigB = 0x61;
        static constexpr uint8_t ConfigC = 0x62;
        static constexpr uint8_t OutXL = 0x68;
    };

    struct Setup {
        static constexpr uint8_t ConfigAValue = 0x8C;
        static constexpr uint8_t ConfigBValue = 0x02;
        static constexpr uint8_t ConfigCValue = 0x10;
        static constexpr int16_t ScaleNumerator = 3;
        static constexpr int16_t ScaleDenominator = 2;
    };
};

}  // namespace drivers::microbit::config
