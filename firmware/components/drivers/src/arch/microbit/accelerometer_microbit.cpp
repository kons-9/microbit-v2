/**
 * @file accelerometer_microbit.cpp
 * @brief micro:bit v2.2 加速度センサ (LSM303AGR) I2C 実装
 */

#include "accelerometer.h"
#include "microbit_driver_config.h"

#define LOG_TAG "ACCEL"
#include "log.h"

#include "nrf_gpio.h"
#include "nrfx_twim.h"

#include <cerrno>

/* ==================================================================
 * Internal state
 * ================================================================== */

namespace {

/*
 * TWIMは物理的に共有されるハードウェア資源であり、I2C転送の間も
 * ハンドルを保持する必要がある。公開APIから隠す内部状態としてまとめる。
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

bool Accelerometer::write_register(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    nrfx_twim_xfer_desc_t xfer
        = NRFX_TWIM_XFER_DESC_TX(microbit::config::Accelerometer::Device::Address, buf, sizeof(buf));
    return nrfx_twim_xfer(&s_state.i2c.twim_instance, &xfer, 0) == 0;
}

bool Accelerometer::read_registers(uint8_t reg, uint8_t *val, uint8_t len) {
    uint8_t reg_addr = reg | microbit::config::Accelerometer::Setup::MultiByteReadMask;
    nrfx_twim_xfer_desc_t xfer
        = NRFX_TWIM_XFER_DESC_TXRX(microbit::config::Accelerometer::Device::Address, &reg_addr, 1, val, len);
    return nrfx_twim_xfer(&s_state.i2c.twim_instance, &xfer, 0) == 0;
}

/* ==================================================================
 * Public API
 * ================================================================== */

bool Accelerometer::init() {
    nrfx_twim_config_t config = NRFX_TWIM_DEFAULT_CONFIG(microbit::config::Accelerometer::Bus::SclPin,
                                                         microbit::config::Accelerometer::Bus::SdaPin);
    config.frequency = microbit::config::Accelerometer::Bus::Frequency;

    if (auto err = nrfx_twim_init(&s_state.i2c.twim_instance, &config, nullptr, nullptr);
        err != 0 && err != -EALREADY) {
        LOG_E("I2C init failed: %d", err);
        return false;
    }
    nrfx_twim_enable(&s_state.i2c.twim_instance);

    uint8_t id = who_am_i();
    if (id != microbit::config::Accelerometer::Device::WhoAmI) {
        LOG_E("WHO_AM_I mismatch: got 0x%02x, expected 0x%02x", id, microbit::config::Accelerometer::Device::WhoAmI);
        return false;
    }

    if (!write_register(microbit::config::Accelerometer::Register::CtrlReg1,
                        microbit::config::Accelerometer::Setup::CtrlReg1Value)) {
        LOG_E("write CTRL_REG1 failed");
        return false;
    }
    if (!write_register(microbit::config::Accelerometer::Register::CtrlReg4,
                        microbit::config::Accelerometer::Setup::CtrlReg4BaseValue)) {
        LOG_E("write CTRL_REG4 failed");
        return false;
    }
    m_state.current_range = AccelerometerRange::G2;

    LOG_D("init ok (WHO_AM_I=0x%02x)", id);
    return true;
}

void Accelerometer::set_range(AccelerometerRange range) {
    const auto range_value = static_cast<uint8_t>(range);
    if (range_value > static_cast<uint8_t>(AccelerometerRange::G16)) {
        return;
    }
    uint8_t ctrl4 = microbit::config::Accelerometer::Setup::CtrlReg4BaseValue | (range_value << 4);
    if (!write_register(microbit::config::Accelerometer::Register::CtrlReg4, ctrl4)) {
        LOG_E("write CTRL_REG4 failed");
        return;
    }
    m_state.current_range = range;
}

AccelerometerData Accelerometer::read() {
    AccelerometerData data = {0, 0, 0};
    uint8_t raw[6];

    if (!read_registers(microbit::config::Accelerometer::Register::OutXL, raw, 6)) {
        return data;
    }

    int16_t raw_x
        = static_cast<int16_t>((raw[1] << 8) | raw[0]) >> microbit::config::Accelerometer::Setup::RawValueShift;
    int16_t raw_y
        = static_cast<int16_t>((raw[3] << 8) | raw[2]) >> microbit::config::Accelerometer::Setup::RawValueShift;
    int16_t raw_z
        = static_cast<int16_t>((raw[5] << 8) | raw[4]) >> microbit::config::Accelerometer::Setup::RawValueShift;

    const auto range_index = static_cast<uint8_t>(m_state.current_range);
    data.m_x = raw_x * microbit::config::Accelerometer::ScaleFactor[range_index];
    data.m_y = raw_y * microbit::config::Accelerometer::ScaleFactor[range_index];
    data.m_z = raw_z * microbit::config::Accelerometer::ScaleFactor[range_index];

    return data;
}

uint8_t Accelerometer::who_am_i() {
    uint8_t id = 0;
    if (!read_registers(microbit::config::Accelerometer::Register::WhoAmI, &id, 1)) {
        LOG_E("read WHO_AM_I failed");
    }
    return id;
}

}  // namespace drivers
