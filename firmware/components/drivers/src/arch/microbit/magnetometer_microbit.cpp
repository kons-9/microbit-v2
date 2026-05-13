/**
 * @file magnetometer_microbit.cpp
 * @brief micro:bit v2.2 地磁気センサ (LSM303AGR) I2C 実装
 */

#include "magnetometer.h"

#define LOG_TAG "MAG"
#include "log.h"

#include "nrf_gpio.h"
#include "nrfx_twim.h"

#include <cerrno>
#include <math.h>

/* ==================================================================
 * Constants
 * ================================================================== */

static constexpr uint32_t I2C_INT_SCL_PIN = NRF_GPIO_PIN_MAP(0, 8);
static constexpr uint32_t I2C_INT_SDA_PIN = NRF_GPIO_PIN_MAP(0, 16);

static constexpr uint8_t LSM303AGR_MAG_ADDR = 0x1E;
static constexpr uint8_t LSM303AGR_MAG_ID = 0x40;

static constexpr uint8_t REG_WHO_AM_I_M = 0x4F;
static constexpr uint8_t REG_CFG_REG_A_M = 0x60;
static constexpr uint8_t REG_CFG_REG_B_M = 0x61;
static constexpr uint8_t REG_CFG_REG_C_M = 0x62;
static constexpr uint8_t REG_OUTX_L_REG_M = 0x68;

/* ==================================================================
 * TWI instance
 * ================================================================== */

static nrfx_twim_t s_twi_instance = NRFX_TWIM_INSTANCE(NRF_TWIM0);

namespace drivers {

/* ==================================================================
 * Private helpers
 * ================================================================== */

bool Magnetometer::write_register(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TX(LSM303AGR_MAG_ADDR, buf, sizeof(buf));
    return nrfx_twim_xfer(&s_twi_instance, &xfer, 0) == 0;
}

bool Magnetometer::read_registers(uint8_t reg, uint8_t *val, uint8_t len) {
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TXRX(LSM303AGR_MAG_ADDR, &reg, 1, val, len);
    return nrfx_twim_xfer(&s_twi_instance, &xfer, 0) == 0;
}

/* ==================================================================
 * Public API
 * ================================================================== */

bool Magnetometer::init() {
    nrfx_twim_config_t config = NRFX_TWIM_DEFAULT_CONFIG(I2C_INT_SCL_PIN, I2C_INT_SDA_PIN);
    config.frequency = NRF_TWIM_FREQ_400K;

    if (auto err = nrfx_twim_init(&s_twi_instance, &config, nullptr, nullptr); err != 0 && err != -EALREADY) {
        LOG_E("I2C init failed: %d", err);
        return false;
    }
    nrfx_twim_enable(&s_twi_instance);

    uint8_t id = who_am_i();
    if (id != LSM303AGR_MAG_ID) {
        LOG_E("WHO_AM_I mismatch: got 0x%02x, expected 0x%02x", id, LSM303AGR_MAG_ID);
        return false;
    }

    if (!write_register(REG_CFG_REG_A_M, 0x8C)) {
        LOG_E("write CFG_REG_A failed");
        return false;
    }
    if (!write_register(REG_CFG_REG_B_M, 0x02)) {
        LOG_E("write CFG_REG_B failed");
        return false;
    }
    if (!write_register(REG_CFG_REG_C_M, 0x10)) {
        LOG_E("write CFG_REG_C failed");
        return false;
    }

    LOG_D("init ok (WHO_AM_I=0x%02x)", id);
    return true;
}

MagnetometerData Magnetometer::read() {
    MagnetometerData data = {0, 0, 0};
    uint8_t raw[6];

    if (!read_registers(REG_OUTX_L_REG_M, raw, 6)) {
        return data;
    }

    auto raw_x = static_cast<int16_t>((raw[1] << 8) | raw[0]);
    auto raw_y = static_cast<int16_t>((raw[3] << 8) | raw[2]);
    auto raw_z = static_cast<int16_t>((raw[5] << 8) | raw[4]);

    data.m_x = (raw_x * 3) / 2;
    data.m_y = (raw_y * 3) / 2;
    data.m_z = (raw_z * 3) / 2;

    return data;
}

uint8_t Magnetometer::who_am_i() {
    uint8_t id = 0;
    if (!read_registers(REG_WHO_AM_I_M, &id, 1)) {
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

} // namespace drivers
