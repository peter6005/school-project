#ifndef BMP280_H
#define BMP280_H

#include "hardware/i2c.h"

void    bmp280_attach(i2c_inst_t *i2c, uint8_t addr);

uint8_t bmp280_read_id(void);

// see docs/datasheet-bmp280.pdf section 3.11.2 table 17
struct bmp280_calib_param {
    uint16_t dig_t1;
    int16_t dig_t2;
    int16_t dig_t3;

    uint16_t dig_p1;
    int16_t dig_p2;
    int16_t dig_p3;
    int16_t dig_p4;
    int16_t dig_p5;
    int16_t dig_p6;
    int16_t dig_p7;
    int16_t dig_p8;
    int16_t dig_p9;
};
void    bmp280_calibrate(struct bmp280_calib_param* params);

int32_t bmp280_read_temp_raw(void);

int32_t bmp280_raw_to_celsius(int32_t raw, struct bmp280_calib_param* params);

#endif