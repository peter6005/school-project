#include <stdio.h>
#include "pico/stdlib.h"

int main()
{
    stdio_init_all();

    // Wait for USB connection
    while (stdio_usb_connected() == false) sleep_ms(100);

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
