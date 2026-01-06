#include "bmp280.h"
#include "lora.h"
#include "config.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int32_t temp;
uint32_t press;
struct bmp280_calib_param params;

int main() {
    stdio_init_all();

    // Wait for USB debug connection
    // while (!stdio_usb_connected()) sleep_ms(100);

    printf("hello world\n");

    bmp280_i2c_init(i2c0, BMP280_SDA_PIN, BMP280_SCL_PIN);
    lora_init();

    uint8_t bmp_id = bmp280_read_id();
    if (bmp_id == BMP280_ID) {
        bmp280_calibrate(&params);
    } else {
        printf("[ERROR] BMP280 not found or unexpected ID. Expected: 0x%02X, actual: 0x%02X\n", BMP280_ID, bmp_id);
        bmp_id = 0; // Indicate unsuccessful initialization
    }

    while (true) {
        if (bmp_id != 0) {
            bmp280_get_temp_and_press(&temp, &press, &params);
            printf("Temperature: %ld.%02ld C, Pressure: %lu.%02lu hPa\n", // %ld.%02d is better than %f for embedded, due to no float support
                   temp / 100, labs(temp % 100), // use labs to avoid -1.23 C --> -1.-23 C scenario
                   press / 100, press % 100);

            char data[50];
            sprintf(data, "T:%ld.%02ld, P:%lu.%02lu", temp / 100, labs(temp % 100), press / 100, press % 100);
            lora_send_packet(data, strlen(data));
        }

        // TODO: NEVER USE SLEEP_MS IN PRODUCTION CODE
        sleep_ms(500);
    }
}
