#include "magnetometer.h"
#include "nrf_gpio.h"
#include "nrfx_twim.h"
#include <math.h>

// micro:bit v2.2 internal I2C bus (shared with accelerometer)
#define I2C_INT_SCL_PIN NRF_GPIO_PIN_MAP(0, 8)
#define I2C_INT_SDA_PIN NRF_GPIO_PIN_MAP(0, 16)

// LSM303AGR magnetometer I2C address
#define LSM303AGR_MAG_ADDR 0x1E

// LSM303AGR magnetometer registers
#define REG_WHO_AM_I_M 0x4F
#define REG_CFG_REG_A_M 0x60
#define REG_CFG_REG_B_M 0x61
#define REG_CFG_REG_C_M 0x62
#define REG_OUTX_L_REG_M 0x68

// WHO_AM_I expected value
#define LSM303AGR_MAG_ID 0x40

// NOTE: I2Cインスタンスはaccelerometerと共有する設計を想定
// 実際の統合時にはI2Cバスマネージャを介してアクセスする
static nrfx_twim_t twi_instance = NRFX_TWIM_INSTANCE(0);

static bool i2c_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TX(LSM303AGR_MAG_ADDR, buf, sizeof(buf));
    return nrfx_twim_xfer(&twi_instance, &xfer, 0) == 0;
}

static bool i2c_read_reg(uint8_t reg, uint8_t *val, uint8_t len) {
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TXRX(LSM303AGR_MAG_ADDR, &reg, 1, val, len);
    return nrfx_twim_xfer(&twi_instance, &xfer, 0) == 0;
}

bool mag_init(void) {
    // I2C初期化 (accelerometerと共有の場合はスキップ)
    nrfx_twim_config_t config = NRFX_TWIM_DEFAULT_CONFIG(I2C_INT_SCL_PIN, I2C_INT_SDA_PIN);
    config.frequency = NRF_TWIM_FREQ_400K;

    // 既に初期化済みの場合はエラーを無視
    nrfx_twim_init(&twi_instance, &config, NULL, NULL);
    nrfx_twim_enable(&twi_instance);

    // WHO_AM_I確認
    if (mag_who_am_i() != LSM303AGR_MAG_ID) {
        return false;
    }

    // CFG_REG_A_M: Continuous mode, ODR=100Hz, temperature compensation ON
    i2c_write_reg(REG_CFG_REG_A_M, 0x8C);

    // CFG_REG_B_M: offset cancellation enabled
    i2c_write_reg(REG_CFG_REG_B_M, 0x02);

    // CFG_REG_C_M: BDU enabled
    i2c_write_reg(REG_CFG_REG_C_M, 0x10);

    return true;
}

mag_data_t mag_read(void) {
    mag_data_t data = {0, 0, 0};
    uint8_t raw[6];

    if (!i2c_read_reg(REG_OUTX_L_REG_M, raw, 6)) {
        return data;
    }

    // 16bit signed, 1.5 mgauss/LSB
    data.x = (int16_t)((raw[1] << 8) | raw[0]);
    data.y = (int16_t)((raw[3] << 8) | raw[2]);
    data.z = (int16_t)((raw[5] << 8) | raw[4]);

    // mGauss単位に変換 (1.5 mgauss/LSB)
    data.x = (data.x * 3) / 2;
    data.y = (data.y * 3) / 2;
    data.z = (data.z * 3) / 2;

    return data;
}

uint8_t mag_who_am_i(void) {
    uint8_t id = 0;
    i2c_read_reg(REG_WHO_AM_I_M, &id, 1);
    return id;
}

uint16_t mag_heading(void) {
    mag_data_t data = mag_read();
    float heading = atan2f((float)data.y, (float)data.x) * 180.0f / 3.14159265f;
    if (heading < 0)
        heading += 360.0f;
    return (uint16_t)heading;
}
