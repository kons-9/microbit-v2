/**
 * @file accelerometer_microbit.cpp
 * @brief micro:bit v2.2 加速度センサ (LSM303AGR) I2C 実装
 */

#include "accelerometer.h"

#define LOG_TAG "ACCEL"
#include "log.h"

#include "nrf_gpio.h"
#include "nrfx_twim.h"

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

/* ==================================================================
 * State
 * ================================================================== */

static nrfx_twim_t s_twiInstance = NRFX_TWIM_INSTANCE(0);
static uint8_t s_currentRange = 0;

static const int16_t s_scaleFactor[] = {1, 2, 4, 12};

/* ==================================================================
 * Internal I2C helpers
 * ================================================================== */

static bool write_register(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TX(LSM303AGR_ACCEL_ADDR, buf, sizeof(buf));
    return nrfx_twim_xfer(&s_twiInstance, &xfer, 0) == 0;
}

static bool read_registers(uint8_t reg, uint8_t *val, uint8_t len) {
    uint8_t regAddr = reg | 0x80;
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TXRX(LSM303AGR_ACCEL_ADDR, &regAddr, 1, val, len);
    return nrfx_twim_xfer(&s_twiInstance, &xfer, 0) == 0;
}

/* ==================================================================
 * API
 * ================================================================== */

bool accelerometer_init(void) {
    nrfx_twim_config_t config = NRFX_TWIM_DEFAULT_CONFIG(I2C_INT_SCL_PIN, I2C_INT_SDA_PIN);
    config.frequency = NRF_TWIM_FREQ_400K;

    if (nrfx_twim_init(&s_twiInstance, &config, nullptr, nullptr) != 0) {
        LOG_E("I2C init failed");
        return false;
    }
    nrfx_twim_enable(&s_twiInstance);

    uint8_t id = accelerometer_who_am_i();
    if (id != LSM303AGR_ACCEL_ID) {
        LOG_E("WHO_AM_I mismatch: got 0x%02x, expected 0x%02x", id, LSM303AGR_ACCEL_ID);
        return false;
    }

    write_register(REG_CTRL_REG1, 0x57);
    write_register(REG_CTRL_REG4, 0x08);
    s_currentRange = 0;

    LOG_D("init ok (WHO_AM_I=0x%02x)", id);
    return true;
}

void accelerometer_set_range(uint8_t range) {
    if (range > 3) {
        return;
    }
    uint8_t ctrl4 = 0x08 | (range << 4);
    write_register(REG_CTRL_REG4, ctrl4);
    s_currentRange = range;
}

AccelerometerData accelerometer_read(void) {
    AccelerometerData data = {0, 0, 0};
    uint8_t raw[6];

    if (!read_registers(REG_OUT_X_L, raw, 6)) {
        return data;
    }

    int16_t rawX = static_cast<int16_t>((raw[1] << 8) | raw[0]) >> 4;
    int16_t rawY = static_cast<int16_t>((raw[3] << 8) | raw[2]) >> 4;
    int16_t rawZ = static_cast<int16_t>((raw[5] << 8) | raw[4]) >> 4;

    data.m_x = rawX * s_scaleFactor[s_currentRange];
    data.m_y = rawY * s_scaleFactor[s_currentRange];
    data.m_z = rawZ * s_scaleFactor[s_currentRange];

    return data;
}

uint8_t accelerometer_who_am_i(void) {
    uint8_t id = 0;
    read_registers(REG_WHO_AM_I, &id, 1);
    return id;
}
