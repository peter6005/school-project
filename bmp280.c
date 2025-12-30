#include "bmp280.h"

#define REG_ID   0xD0

static i2c_inst_t *bmp_i2c;
static uint8_t bmp_addr;

void bmp280_attach(i2c_inst_t *i2c, uint8_t addr) {
    bmp_i2c = i2c;
    bmp_addr = addr;
}

uint8_t bmp280_read_id(void) {
    uint8_t reg = REG_ID;
    uint8_t val;
    i2c_write_blocking(bmp_i2c, bmp_addr, &reg, 1, true);
    i2c_read_blocking(bmp_i2c, bmp_addr, &val, 1, false);
    return val;
}