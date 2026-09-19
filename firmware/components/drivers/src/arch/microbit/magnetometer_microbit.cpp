/**
 * @file magnetometer_microbit.cpp
 * @brief micro:bit v2.2 地磁気センサ (LSM303AGR) I2C 実装
 */

#include "magnetometer.h"
#include "microbit_driver_config.h"

#define LOG_TAG "MAG"
#include "log.h"

#include "nrf_gpio.h"
#include "nrfx_twim.h"

#include <cerrno>
#include <math.h>

/* ==================================================================
 * Internal state
 * ================================================================== */

namespace {

/*
 * 加速度計と同じTWIMペリフェラルを使うため、ハードウェアハンドルを
 * 内部状態として保持する。これは公開APIの利用者が直接扱う状態ではない。
 */
struct InnerState {
    struct I2c {
        nrfx_twim_t twim_instance = NRFX_TWIM_INSTANCE(NRF_TWIM0);
    } i2c;
};

static InnerState s_state{};

}  // namespace

namespace drivers {

/* ==================================================================
 * Private helpers
 * ================================================================== */

bool Magnetometer::write_register(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    nrfx_twim_xfer_desc_t xfer
        = NRFX_TWIM_XFER_DESC_TX(microbit::config::Magnetometer::Device::Address, buf, sizeof(buf));
    return nrfx_twim_xfer(&s_state.i2c.twim_instance, &xfer, 0) == 0;
}

bool Magnetometer::read_registers(uint8_t reg, uint8_t *val, uint8_t len) {
    nrfx_twim_xfer_desc_t xfer
        = NRFX_TWIM_XFER_DESC_TXRX(microbit::config::Magnetometer::Device::Address, &reg, 1, val, len);
    return nrfx_twim_xfer(&s_state.i2c.twim_instance, &xfer, 0) == 0;
}

/* ==================================================================
 * Public API
 * ================================================================== */

bool Magnetometer::init() {
    nrfx_twim_config_t config = NRFX_TWIM_DEFAULT_CONFIG(microbit::config::Magnetometer::Bus::SclPin,
                                                          microbit::config::Magnetometer::Bus::SdaPin);
    config.frequency = microbit::config::Magnetometer::Bus::Frequency;

    if (auto err = nrfx_twim_init(&s_state.i2c.twim_instance, &config, nullptr, nullptr); err != 0 && err != -EALREADY) {
        LOG_E("I2C init failed: %d", err);
        return false;
    }
    nrfx_twim_enable(&s_state.i2c.twim_instance);

    uint8_t id = who_am_i();
    if (id != microbit::config::Magnetometer::Device::WhoAmI) {
        LOG_E("WHO_AM_I mismatch: got 0x%02x, expected 0x%02x", id, microbit::config::Magnetometer::Device::WhoAmI);
        return false;
    }

    if (!write_register(microbit::config::Magnetometer::Register::ConfigA,
                        microbit::config::Magnetometer::Setup::ConfigAValue)) {
        LOG_E("write CFG_REG_A failed");
        return false;
    }
    if (!write_register(microbit::config::Magnetometer::Register::ConfigB,
                        microbit::config::Magnetometer::Setup::ConfigBValue)) {
        LOG_E("write CFG_REG_B failed");
        return false;
    }
    if (!write_register(microbit::config::Magnetometer::Register::ConfigC,
                        microbit::config::Magnetometer::Setup::ConfigCValue)) {
        LOG_E("write CFG_REG_C failed");
        return false;
    }

    LOG_D("init ok (WHO_AM_I=0x%02x)", id);
    return true;
}

MagnetometerData Magnetometer::read() {
    MagnetometerData data = {0, 0, 0};
    uint8_t raw[6];

    if (!read_registers(microbit::config::Magnetometer::Register::OutXL, raw, 6)) {
        return data;
    }

    auto raw_x = static_cast<int16_t>((raw[1] << 8) | raw[0]);
    auto raw_y = static_cast<int16_t>((raw[3] << 8) | raw[2]);
    auto raw_z = static_cast<int16_t>((raw[5] << 8) | raw[4]);

    data.m_x = (raw_x * microbit::config::Magnetometer::Setup::ScaleNumerator)
               / microbit::config::Magnetometer::Setup::ScaleDenominator;
    data.m_y = (raw_y * microbit::config::Magnetometer::Setup::ScaleNumerator)
               / microbit::config::Magnetometer::Setup::ScaleDenominator;
    data.m_z = (raw_z * microbit::config::Magnetometer::Setup::ScaleNumerator)
               / microbit::config::Magnetometer::Setup::ScaleDenominator;

    return data;
}

uint8_t Magnetometer::who_am_i() {
    uint8_t id = 0;
    if (!read_registers(microbit::config::Magnetometer::Register::WhoAmI, &id, 1)) {
        LOG_E("read WHO_AM_I failed");
    }
    return id;
}

uint16_t Magnetometer::heading() {
    auto data = read();
    float h = atan2f(static_cast<float>(data.m_y), static_cast<float>(data.m_x)) * 180.0f / 3.14159265f;
    if (h < 0.0f) {
        h += 360.0f;
    }
    return static_cast<uint16_t>(h);
}

}  // namespace drivers
