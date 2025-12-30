#include "bmp280.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>

#define SDA_PIN 4
#define SCL_PIN 5
#define BMP280_ADDR 0x76
#define BMP280_ID 0x58
#define PICO_ONBOARD_LED_PIN 25

int main() {
    stdio_init_all();

    // Wait for USB debug connection
    while (!stdio_usb_connected()) sleep_ms(100);

    printf("BMP280 test\n");

    // Standart 100kHz I2C init at I2C0 for BMP280
    i2c_init(i2c0, 100000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    bmp280_attach(i2c0, BMP280_ADDR);
    struct bmp280_calib_param params;

    uint8_t bmp_id = bmp280_read_id();
    if (bmp_id == BMP280_ID) {
        bmp280_calibrate(&params);
    } else {
        printf("[ERROR] BMP280 not found or unexpected ID. Expected: 0x%02X, actual: 0x%02X\n", BMP280_ID, bmp_id);
        bmp_id = 0; // Indicate unsuccessful initialization
    }

    while (true) {
        if (bmp_id != 0) {
            int32_t temp;
            uint32_t press;

            bmp280_get_temp_and_press(&temp, &press, &params);
            printf("Temperature: %ld.%02ld C, Pressure: %lu.%02lu hPa\n", // %ld.%02d is better than %f for embedded, due to no float support
                   temp / 100, labs(temp % 100), // use labs to avoid -1.23 C --> -1.-23 C scenario
                   press / 100, press % 100);
        }

        sleep_ms(500);
    }
}
