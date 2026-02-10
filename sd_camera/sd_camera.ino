#include "WiFi.h"
#include <eloquent_esp32cam.h>
#include <eloquent_esp32cam/extra/esp32/fs/sdmmc.h>

using namespace eloq;

#define LED_PIN 2

void setup() {
    // kill radios asap
    WiFi.mode(WIFI_OFF);
    WiFi.disconnect(true);
    btStop();

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    camera.pinout.esp32_s3_cam();
    camera.resolution.uxga();
    camera.quality.best();

    sdmmc.pinout.clk(39);
    sdmmc.pinout.cmd(38);
    sdmmc.pinout.d0(40);

    while (!camera.begin().isOk()) {}
    while (!sdmmc.begin().isOk()) {}
}

void loop() {
    if (!camera.capture().isOk()) {
        return;
    }

    digitalWrite(LED_PIN, HIGH);
    sdmmc.save(camera.frame).to("", "jpg");
    digitalWrite(LED_PIN, LOW);
    delay(100);
}
