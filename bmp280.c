#include "bmp280.h"

#define REG_ID        0xD0
#define REG_TEMP      0xFA
#define REG_CALIB     0x88
#define REG_CTRL_MEAS 0xF4

static i2c_inst_t *bmp_i2c;
static uint8_t bmp_addr;

// Calibration parameters
static uint16_t dig_T1;
static int16_t  dig_T2;
static int16_t  dig_T3;

/* -------- I2C helper -------- */
static void bmp_read(uint8_t reg, uint8_t *buf, uint8_t len) {
    i2c_write_blocking(bmp_i2c, bmp_addr, &reg, 1, true);
    i2c_read_blocking(bmp_i2c, bmp_addr, buf, len, false);
}
static void bmp_write8(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    i2c_write_blocking(bmp_i2c, bmp_addr, buf, 2, false);
}
/* ---------------------------- */

void bmp280_attach(i2c_inst_t *i2c, uint8_t addr) {
    bmp_i2c = i2c;
    bmp_addr = addr;
}

uint8_t bmp280_read_id(void) {
    uint8_t id;
    bmp_read(REG_ID, &id, 1);
    return id;
}

void bmp280_calibrate(void) {
    uint8_t buf[6];

    bmp_read(REG_CALIB, buf, 6);

    dig_T1 = (uint16_t)(buf[1] << 8 | buf[0]);
    dig_T2 = (int16_t) (buf[3] << 8 | buf[2]);
    dig_T3 = (int16_t) (buf[5] << 8 | buf[4]);

    bmp_write8(REG_CTRL_MEAS, 0x27);
}

int32_t bmp280_read_temp_raw(void) {
    uint8_t reg = REG_TEMP;
    uint8_t buf[3];

    i2c_write_blocking(bmp_i2c, bmp_addr, &reg, 1, true);
    i2c_read_blocking(bmp_i2c, bmp_addr, buf, 3, false);

    return (buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4);
}

int32_t bmp280_raw_to_celsius(int32_t raw_temp)
{
    int32_t diff1 = (raw_temp >> 3) - ((int32_t)dig_T1 << 1);
    int32_t part1 = (diff1 * (int32_t)dig_T2) >> 11;

    int32_t diff2 = (raw_temp >> 4) - (int32_t)dig_T1;
    int32_t part2 = ((diff2 * diff2) >> 12) * (int32_t)dig_T3 >> 14;

    int32_t t_fine = part1 + part2;

    int32_t temp_centi = (t_fine * 5 + 128) >> 8;

    return temp_centi;
}