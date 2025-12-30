#include "bmp280.h"

#define REG_ID         0xD0
#define REG_TEMP       0xFA
#define REG_CALIB      0x88
#define REG_CTRL_MEAS  0xF4
#define REG_DIG_T1_LSB 0x88

static i2c_inst_t *bmp_i2c;
static uint8_t bmp_addr;

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

// see docs/datasheet-bmp280.pdf section 4.3.1:
// 4.3.1 Register 0xD0 “id”
// The “id” register contains the chip identification number chip_id[7:0], which is 0x58.
// This number can be read as soon as the device finished the power-on-reset.
uint8_t bmp280_read_id(void) {
    uint8_t id;
    bmp_read(REG_ID, &id, 1);
    return id;
}

void bmp280_calibrate(struct bmp280_calib_param* params) {
    // see docs/datasheet-bmp280.pdf section 4.3.4
    // our config:
    // ultra low power mode, temp oversampling x1,
    // pressure oversampling 1x, forced mode
    bmp_write8(REG_CTRL_MEAS, 0x2F);

    uint8_t buf[24];
    bmp_write8(REG_DIG_T1_LSB, 0);
    bmp_read(REG_DIG_T1_LSB, buf, 24);

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
}

int32_t bmp280_read_temp_raw(void) {
    uint8_t reg = REG_TEMP;
    uint8_t buf[3];

    i2c_write_blocking(bmp_i2c, bmp_addr, &reg, 1, true);
    i2c_read_blocking(bmp_i2c, bmp_addr, buf, 3, false);

    return (buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4);
}

int32_t bmp280_raw_to_celsius(int32_t raw_temp, struct bmp280_calib_param* params)
{
    int32_t diff1 = (raw_temp >> 3) - ((int32_t)params->dig_t1 << 1);
    int32_t part1 = (diff1 * (int32_t)params->dig_t2) >> 11;

    int32_t diff2 = (raw_temp >> 4) - (int32_t)params->dig_t1;
    int32_t part2 = ((diff2 * diff2) >> 12) * (int32_t)params->dig_t3 >> 14;
    int32_t t_fine = part1 + part2;

    int32_t temp_centi = (t_fine * 5 + 128) >> 8;

    return temp_centi;
}