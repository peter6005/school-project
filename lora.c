#include "lora.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <string.h>

static void cs_select(void) {
    gpio_put(CS_PIN, 0);
}

static void cs_deselect(void) {
    gpio_put(CS_PIN, 1);
}

void lora_init(void) {
    gpio_init(CS_PIN);
    gpio_set_dir(CS_PIN, GPIO_OUT);
    cs_deselect();

    gpio_init(RST_PIN);
    gpio_set_dir(RST_PIN, GPIO_OUT);

    spi_init(SPI_PORT, 1000000); // 1 MHz
    gpio_set_function(SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MISO_PIN, GPIO_FUNC_SPI);

    lora_reset();

    // copy-paste code, seems working although not fully understood right now
    // need to revisit later
    lora_write_reg(REG_OP_MODE, MODE_LORA | MODE_SLEEP);
    lora_write_reg(REG_FR_MSB, FR_MSB); // 868.1 MHz
    lora_write_reg(REG_FR_MID, FR_MID);
    lora_write_reg(REG_FR_LSB, FR_LSB);
    lora_write_reg(REG_PA_CONFIG, PA_CONFIG_MAX); // Max power
    lora_write_reg(REG_FIFO_TX_BASE_ADDR, 0x00); // FIFO TX base address
    lora_write_reg(REG_OP_MODE, MODE_LORA | MODE_STDBY); // Standby mode
}

void lora_reset(void) {
    gpio_put(RST_PIN, 0);
    sleep_ms(50);
    gpio_put(RST_PIN, 1);
    sleep_ms(50);
}

void lora_write_reg(uint8_t reg, uint8_t val) {
    cs_select();
    uint8_t buf[2] = {reg | 0x80, val};
    spi_write_blocking(SPI_PORT, buf, 2);
    cs_deselect();
}

void lora_send_packet(const char* data, uint8_t len) {
    lora_write_reg(REG_FIFO_ADDR_PTR, 0x00);

    cs_select();
    uint8_t fifo_reg = REG_FIFO | 0x80;
    spi_write_blocking(SPI_PORT, &fifo_reg, 1);
    spi_write_blocking(SPI_PORT, (uint8_t*)data, len);
    cs_deselect();

    lora_write_reg(REG_PAYLOAD_LENGTH, len);
    lora_write_reg(REG_OP_MODE, MODE_LORA | MODE_TX);

    sleep_ms(100);

    lora_write_reg(REG_OP_MODE, MODE_LORA | MODE_STDBY);
}