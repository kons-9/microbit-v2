/**
 * @file accelerometer_microbit.cpp
 * @brief micro:bit v2.2 加速度センサ (LSM303AGR) I2C 実装
 */

#include "accelerometer.h"

#define LOG_TAG "ACCEL"
#include "log.h"

#include "nrf_gpio.h"
#include "nrfx_twim.h"

#include <cerrno>

/* ==================================================================
 * Constants
 * ================================================================== */

static constexpr uint32_t I2C_INT_SCL_PIN = NRF_GPIO_PIN_MAP(0, 8);
static constexpr uint32_t I2C_INT_SDA_PIN = NRF_GPIO_PIN_MAP(0, 16);

static constexpr uint8_t LSM303AGR_ACCEL_ADDR = 0x19;
static constexpr uint8_t LSM303AGR_ACCEL_ID = 0x33;

static constexpr uint8_t REG_WHO_AM_I = 0x0F;
static constexpr uint8_t REG_CTRL_REG1 = 0x20;
static constexpr uint8_t REG_CTRL_REG4 = 0x23;
static constexpr uint8_t REG_OUT_X_L = 0x28;

static constexpr int16_t SCALE_FACTOR[] = {1, 2, 4, 12};

/* ==================================================================
 * TWI instance (shared across Accelerometer instances)
 * ================================================================== */

static nrfx_twim_t s_twi_instance = NRFX_TWIM_INSTANCE(NRF_TWIM0);

namespace drivers {

/* ==================================================================
 * Private helpers
 * ================================================================== */

bool Accelerometer::write_register(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TX(LSM303AGR_ACCEL_ADDR, buf, sizeof(buf));
    return nrfx_twim_xfer(&s_twi_instance, &xfer, 0) == 0;
}

bool Accelerometer::read_registers(uint8_t reg, uint8_t *val, uint8_t len) {
    uint8_t reg_addr = reg | 0x80;
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TXRX(LSM303AGR_ACCEL_ADDR, &reg_addr, 1, val, len);
    return nrfx_twim_xfer(&s_twi_instance, &xfer, 0) == 0;
}

/* ==================================================================
 * Public API
 * ================================================================== */

bool Accelerometer::init() {
    nrfx_twim_config_t config = NRFX_TWIM_DEFAULT_CONFIG(I2C_INT_SCL_PIN, I2C_INT_SDA_PIN);
    config.frequency = NRF_TWIM_FREQ_400K;

    if (auto err = nrfx_twim_init(&s_twi_instance, &config, nullptr, nullptr); err != 0 && err != -EALREADY) {
        LOG_E("I2C init failed: %d", err);
        return false;
    }
    nrfx_twim_enable(&s_twi_instance);

    uint8_t id = who_am_i();
    if (id != LSM303AGR_ACCEL_ID) {
        LOG_E("WHO_AM_I mismatch: got 0x%02x, expected 0x%02x", id, LSM303AGR_ACCEL_ID);
        return false;
    }

    if (!write_register(REG_CTRL_REG1, 0x57)) {
        LOG_E("write CTRL_REG1 failed");
        return false;
    }
    if (!write_register(REG_CTRL_REG4, 0x08)) {
        LOG_E("write CTRL_REG4 failed");
        return false;
    }
    m_current_range = AccelerometerRange::G2;

    LOG_D("init ok (WHO_AM_I=0x%02x)", id);
    return true;
}

void Accelerometer::set_range(AccelerometerRange range) {
    const auto range_value = static_cast<uint8_t>(range);
    if (range_value > static_cast<uint8_t>(AccelerometerRange::G16)) {
        return;
    }
    uint8_t ctrl4 = 0x08 | (range_value << 4);
    if (!write_register(REG_CTRL_REG4, ctrl4)) {
        LOG_E("write CTRL_REG4 failed");
        return;
    }
    m_current_range = range;
}

AccelerometerData Accelerometer::read() {
    AccelerometerData data = {0, 0, 0};
    uint8_t raw[6];

    if (!read_registers(REG_OUT_X_L, raw, 6)) {
        return data;
    }

    int16_t raw_x = static_cast<int16_t>((raw[1] << 8) | raw[0]) >> 4;
    int16_t raw_y = static_cast<int16_t>((raw[3] << 8) | raw[2]) >> 4;
    int16_t raw_z = static_cast<int16_t>((raw[5] << 8) | raw[4]) >> 4;

    const auto range_index = static_cast<uint8_t>(m_current_range);
    data.m_x = raw_x * SCALE_FACTOR[range_index];
    data.m_y = raw_y * SCALE_FACTOR[range_index];
    data.m_z = raw_z * SCALE_FACTOR[range_index];

    return data;
}

uint8_t Accelerometer::who_am_i() {
    uint8_t id = 0;
    if (!read_registers(REG_WHO_AM_I, &id, 1)) {
        LOG_E("read WHO_AM_I failed");
    }
    return id;
}

}  // namespace drivers
