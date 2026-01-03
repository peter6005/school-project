#include "bmp280.h"
#include "pico/stdlib.h"

#define REG_ID         0xD0
#define REG_TEMP       0xFA
#define REG_PRESS      0xF7
#define REG_CALIB      0x88
#define REG_STATUS     0xF3
#define REG_CTRL_MEAS  0xF4
#define BMP280_ADDR    0x76

static i2c_inst_t *bmp_i2c;

/* -------- I2C helper -------- */
static inline void bmp_read(uint8_t reg, uint8_t *buf, size_t len) {
    i2c_write_blocking(bmp_i2c, BMP280_ADDR, &reg, 1, true);
    i2c_read_blocking(bmp_i2c, BMP280_ADDR, buf, len, false);
}
static inline void bmp_write8(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    i2c_write_blocking(bmp_i2c, BMP280_ADDR, buf, 2, false);
}
/* ---------------------------- */

void bmp280_i2c_init(i2c_inst_t *i2c, uint8_t sda, uint8_t scl) {
    // Standart 100kHz I2C init at I2C0 for BMP280
    i2c_init(i2c, 100000);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
    bmp_i2c = i2c;
}

// see docs/datasheet-bmp280.pdf section 4.3.1:
// 4.3.1 Register 0xD0 “id”
// The “id” register contains the chip identification number chip_id[7:0], which is 0x58.
// This number can be read as soon as the device finished the power-on-reset.
uint8_t bmp280_read_id(void) {
    uint8_t id;
    bmp_read(REG_ID, &id, 1);
    return id;
}

void bmp280_trigger_measurement(void) {
    // set forced mode, temp and press oversampling x1
    bmp_write8(REG_CTRL_MEAS, 0x2F);

    // wait until measurement is complete
    uint8_t status;
    do {
        bmp_read(REG_STATUS, &status, 1);
        tight_loop_contents(); // better alternative than sleep_ms
    } while (status & 0x08);
}

void bmp280_calibrate(struct bmp280_calib_param* params) {
    // get calibration data from the sensor
    uint8_t buf[24];
    bmp_read(REG_CALIB, buf, 24);

    // store in a struct for later calculations
    params->dig_t1 = (uint16_t)(buf[1] << 8) | buf[0];
    params->dig_t2 = (int16_t)(buf[3] << 8) | buf[2];
    params->dig_t3 = (int16_t)(buf[5] << 8) | buf[4];

    params->dig_p1 = (uint16_t)(buf[7] << 8) | buf[6];
    params->dig_p2 = (int16_t)(buf[9] << 8) | buf[8];
    params->dig_p3 = (int16_t)(buf[11] << 8) | buf[10];
    params->dig_p4 = (int16_t)(buf[13] << 8) | buf[12];
    params->dig_p5 = (int16_t)(buf[15] << 8) | buf[14];
    params->dig_p6 = (int16_t)(buf[17] << 8) | buf[16];
    params->dig_p7 = (int16_t)(buf[19] << 8) | buf[18];
    params->dig_p8 = (int16_t)(buf[21] << 8) | buf[20];
    params->dig_p9 = (int16_t)(buf[23] << 8) | buf[22];

    // dummy read to warm up the sensor (first measurement is usually inaccurate)
    bmp280_trigger_measurement();
    (void)bmp280_read_temp_raw();
    (void)bmp280_read_press_raw();
}

int32_t bmp280_read_temp_raw(void) {
    uint8_t buf[3];

    bmp_read(REG_TEMP, buf, 3);

    return ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) |((int32_t)buf[2] >> 4);
}

int32_t bmp280_read_press_raw(void)
{
    uint8_t buf[3];
    bmp_read(REG_PRESS, buf, 3);

    return ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) |((int32_t)buf[2] >> 4);
}

int32_t bmp280_raw_to_celsius(int32_t raw_temp, struct bmp280_calib_param *params, int32_t *t_fine)
{
    int32_t diff1 = (raw_temp >> 3) - ((int32_t)params->dig_t1 << 1);
    int32_t part1 = (diff1 * (int32_t)params->dig_t2) >> 11;

    int32_t diff2 = (raw_temp >> 4) - (int32_t)params->dig_t1;
    int32_t part2 = ((diff2 * diff2) >> 12) * (int32_t)params->dig_t3 >> 14;
    *t_fine = part1 + part2;

    int32_t temp_centi = (*t_fine * 5 + 128) >> 8;

    return temp_centi;
}

uint32_t bmp280_raw_to_pressure(int32_t raw_press, struct bmp280_calib_param *p, int32_t t_fine)
{
    int64_t temp_offset = (int64_t)t_fine - 128000;
    int64_t temp_sq = temp_offset * temp_offset;

    int64_t pressure_offset =
        (temp_sq * p->dig_p6) +
        (temp_offset * p->dig_p5 << 17) +
        ((int64_t)p->dig_p4 << 35);

    int64_t pressure_scale =
        ((temp_sq * p->dig_p3) >> 8) +
        ((temp_offset * p->dig_p2) << 12);

    pressure_scale =
        (((int64_t)1 << 47) + pressure_scale) * p->dig_p1 >> 33;

    // avoid division by zero (it shouldn't supposed to happen, but who knows)
    if (pressure_scale == 0) {
        return 0;
    }

    // 2^20 ADC resolution
    int64_t pressure = 1048576 - raw_press;

    pressure = ((pressure << 31) - pressure_offset) * 3125 / pressure_scale;

    int64_t pressure_sq = (pressure >> 13) * (pressure >> 13);

    int64_t correction =
        (pressure_sq * p->dig_p9 >> 25) +
        (pressure * p->dig_p8 >> 19);

    pressure = (pressure + correction) >> 8;
    pressure += ((int64_t)p->dig_p7 << 4);

    return (uint32_t)((pressure * 100) >> 8);
}

void bmp280_get_temp_and_press(int32_t *temp_centi, uint32_t *press_pa, struct bmp280_calib_param* params)
{
    int32_t t_fine;

    // due to forced mode, we need to trigger measurement each time
    bmp280_trigger_measurement();

    int32_t raw_temp = bmp280_read_temp_raw();
    *temp_centi = bmp280_raw_to_celsius(raw_temp, params, &t_fine);

    int32_t raw_press = bmp280_read_press_raw();
    *press_pa = bmp280_raw_to_pressure(raw_press, params, t_fine);
}