/**
 * @file magnetometer_microbit.cpp
 * @brief micro:bit v2.2 地磁気センサ (LSM303AGR) I2C 実装
 */

#include "magnetometer.h"

#include "nrf_gpio.h"
#include "nrfx_twim.h"

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
 * State
 * NOTE: I2C インスタンスは accelerometer と共有する設計を想定
 * ================================================================== */

static nrfx_twim_t s_twiInstance = NRFX_TWIM_INSTANCE(0);

/* ==================================================================
 * Internal I2C helpers
 * ================================================================== */

static bool write_register(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TX(LSM303AGR_MAG_ADDR, buf, sizeof(buf));
    return nrfx_twim_xfer(&s_twiInstance, &xfer, 0) == 0;
}

static bool read_registers(uint8_t reg, uint8_t *val, uint8_t len) {
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TXRX(LSM303AGR_MAG_ADDR, &reg, 1, val, len);
    return nrfx_twim_xfer(&s_twiInstance, &xfer, 0) == 0;
}

/* ==================================================================
 * API
 * ================================================================== */

bool magnetometer_init(void) {
    nrfx_twim_config_t config = NRFX_TWIM_DEFAULT_CONFIG(I2C_INT_SCL_PIN, I2C_INT_SDA_PIN);
    config.frequency = NRF_TWIM_FREQ_400K;

    nrfx_twim_init(&s_twiInstance, &config, nullptr, nullptr);
    nrfx_twim_enable(&s_twiInstance);

    if (magnetometer_who_am_i() != LSM303AGR_MAG_ID) {
        return false;
    }

    write_register(REG_CFG_REG_A_M, 0x8C);
    write_register(REG_CFG_REG_B_M, 0x02);
    write_register(REG_CFG_REG_C_M, 0x10);

    return true;
}

MagnetometerData magnetometer_read(void) {
    MagnetometerData data = {0, 0, 0};
    uint8_t raw[6];

    if (!read_registers(REG_OUTX_L_REG_M, raw, 6)) {
        return data;
    }

    auto rawX = static_cast<int16_t>((raw[1] << 8) | raw[0]);
    auto rawY = static_cast<int16_t>((raw[3] << 8) | raw[2]);
    auto rawZ = static_cast<int16_t>((raw[5] << 8) | raw[4]);

    data.m_x = (rawX * 3) / 2;
    data.m_y = (rawY * 3) / 2;
    data.m_z = (rawZ * 3) / 2;

    return data;
}

uint8_t magnetometer_who_am_i(void) {
    uint8_t id = 0;
    read_registers(REG_WHO_AM_I_M, &id, 1);
    return id;
}

uint16_t magnetometer_heading(void) {
    auto data = magnetometer_read();
    float heading = atan2f(static_cast<float>(data.m_y), static_cast<float>(data.m_x)) * 180.0f / 3.14159265f;
    if (heading < 0.0f) {
        heading += 360.0f;
    }
    return static_cast<uint16_t>(heading);
}
