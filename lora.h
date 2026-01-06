#ifndef LORA_H
#define LORA_H

#include "hardware/spi.h"
#include <stdint.h>
#include <stdbool.h>

#define SPI_PORT spi0
#define CS_PIN 17
#define RST_PIN 21
#define SCK_PIN 18
#define MOSI_PIN 19
#define MISO_PIN 16

#define REG_FIFO 0x00
#define REG_OP_MODE 0x01
#define REG_FR_MSB 0x06
#define REG_FR_MID 0x07
#define REG_FR_LSB 0x08
#define REG_PA_CONFIG 0x09
#define REG_FIFO_TX_BASE_ADDR 0x0E
#define REG_FIFO_ADDR_PTR 0x0D
#define REG_PAYLOAD_LENGTH 0x22

#define MODE_SLEEP 0x00
#define MODE_STDBY 0x01
#define MODE_TX 0x03
#define MODE_LORA 0x80

// frequency for 868.1 MHz
#define FR_MSB 0xD9
#define FR_MID 0x06
#define FR_LSB 0x66

#define PA_CONFIG_MAX 0x8F

void lora_init(void);
void lora_reset(void);
void lora_write_reg(uint8_t reg, uint8_t val);
void lora_send_packet(const char* data, uint8_t len);

#endif