#ifndef BMP280_H
#define BMP280_H

#include "hardware/i2c.h"

void    bmp280_attach(i2c_inst_t *i2c, uint8_t addr);

uint8_t bmp280_read_id(void);

void    bmp280_calibrate(void);

int32_t bmp280_read_temp_raw(void);

int32_t bmp280_raw_to_celsius(int32_t raw);

#endif