#include "accelerometer.h"
#include "nrf_gpio.h"
#include "nrfx_twim.h"

// micro:bit v2.2 internal I2C bus
#define I2C_INT_SCL_PIN NRF_GPIO_PIN_MAP(0, 8)
#define I2C_INT_SDA_PIN NRF_GPIO_PIN_MAP(0, 16)

// LSM303AGR accelerometer I2C address
#define LSM303AGR_ACCEL_ADDR 0x19

// LSM303AGR accelerometer registers
#define REG_WHO_AM_I 0x0F
#define REG_CTRL_REG1 0x20
#define REG_CTRL_REG4 0x23
#define REG_OUT_X_L 0x28

// WHO_AM_I expected value
#define LSM303AGR_ACCEL_ID 0x33

static nrfx_twim_t twi_instance = NRFX_TWIM_INSTANCE(0);
static accel_range_t current_range = ACCEL_RANGE_2G;

// mg/LSBスケールファクタ (12bit mode)
static const int16_t scale_factor[] = {
    1,   // ±2g:  1 mg/LSB
    2,   // ±4g:  2 mg/LSB
    4,   // ±8g:  4 mg/LSB
    12,  // ±16g: 12 mg/LSB
};

static bool i2c_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TX(LSM303AGR_ACCEL_ADDR, buf, sizeof(buf));
    return nrfx_twim_xfer(&twi_instance, &xfer, 0) == 0;
}

static bool i2c_read_reg(uint8_t reg, uint8_t *val, uint8_t len) {
    // MSBセット = auto-increment for multi-byte read
    uint8_t reg_addr = reg | 0x80;
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TXRX(LSM303AGR_ACCEL_ADDR, &reg_addr, 1, val, len);
    return nrfx_twim_xfer(&twi_instance, &xfer, 0) == 0;
}

bool accel_init(void) {
    // I2C初期化
    nrfx_twim_config_t config = NRFX_TWIM_DEFAULT_CONFIG(I2C_INT_SCL_PIN, I2C_INT_SDA_PIN);
    config.frequency = NRF_TWIM_FREQ_400K;

    if (nrfx_twim_init(&twi_instance, &config, NULL, NULL) != 0) {
        return false;
    }
    nrfx_twim_enable(&twi_instance);

    // WHO_AM_I確認
    if (accel_who_am_i() != LSM303AGR_ACCEL_ID) {
        return false;
    }

    // CTRL_REG1: ODR=100Hz, all axes enabled, normal mode
    i2c_write_reg(REG_CTRL_REG1, 0x57);

    // CTRL_REG4: ±2g, high-resolution (12bit)
    i2c_write_reg(REG_CTRL_REG4, 0x08);
    current_range = ACCEL_RANGE_2G;

    return true;
}

void accel_set_range(accel_range_t range) {
    uint8_t ctrl4 = 0x08;  // HR bit
    ctrl4 |= ((uint8_t)range << 4);
    i2c_write_reg(REG_CTRL_REG4, ctrl4);
    current_range = range;
}

accel_data_t accel_read(void) {
    accel_data_t data = {0, 0, 0};
    uint8_t raw[6];

    if (!i2c_read_reg(REG_OUT_X_L, raw, 6)) {
        return data;
    }

    // 12bit left-justified in 16bit -> shift right 4
    int16_t raw_x = (int16_t)((raw[1] << 8) | raw[0]) >> 4;
    int16_t raw_y = (int16_t)((raw[3] << 8) | raw[2]) >> 4;
    int16_t raw_z = (int16_t)((raw[5] << 8) | raw[4]) >> 4;

    // mg に変換
    data.x = raw_x * scale_factor[current_range];
    data.y = raw_y * scale_factor[current_range];
    data.z = raw_z * scale_factor[current_range];

    return data;
}

uint8_t accel_who_am_i(void) {
    uint8_t id = 0;
    i2c_read_reg(REG_WHO_AM_I, &id, 1);
    return id;
}
