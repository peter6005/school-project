#include "bmp280.h"
#include "pico/stdlib.h"
#include <stdio.h>

#define SDA_PIN 4
#define SCL_PIN 5
#define BMP280_ADDR 0x76
#define BMP280_ID 0x58
#define PICO_ONBOARD_LED_PIN 25

int main() {
    stdio_init_all();

    // Wait for USB debug connection
    while (!stdio_usb_connected()) sleep_ms(100);

    printf("BMP280 initialization test\n");

    // Standart 100kHz I2C init at I2C0 for BMP280
    i2c_init(i2c0, 100000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    bmp280_attach(i2c0, BMP280_ADDR);

    // Read and verify BMP280 ID
    uint8_t bmp_id = bmp280_read_id();
    if (bmp_id != BMP280_ID) {
        printf("BMP280 not found or unexpected ID. Expected: 0x%02X, actual: 0x%02X\n", BMP280_ID, bmp_id);
        bmp_id = 0; // Indicate unsuccessful initialization
    }

    gpio_init(PICO_ONBOARD_LED_PIN);
    gpio_set_dir(PICO_ONBOARD_LED_PIN, GPIO_OUT);

    while (true) {
        // blink LED to show activity
        gpio_put(PICO_ONBOARD_LED_PIN, 1);
        sleep_ms(100);
        gpio_put(PICO_ONBOARD_LED_PIN, 0);
        sleep_ms(100);
    }
}
