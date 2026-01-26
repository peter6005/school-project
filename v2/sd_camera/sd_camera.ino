#include "WiFi.h"
#include <eloquent_esp32cam.h>
#include <eloquent_esp32cam/extra/esp32/fs/sdmmc.h>

using namespace eloq;

#define TRIGGER_PIN 47
#define LED_PIN     2

static uint32_t imgCounter = 0;
static int lastState = HIGH;

void setup() {
    // kill radios asap
    WiFi.mode(WIFI_OFF);
    WiFi.disconnect(true);
    btStop();

    pinMode(TRIGGER_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    camera.pinout.esp32_s3_cam();
    camera.resolution.uxga();
    camera.quality.best();

    while (!camera.begin().isOk()) { }

    sdmmc.pinout.clk(39);
    sdmmc.pinout.cmd(38);
    sdmmc.pinout.d0(40);

    while (!sdmmc.begin().isOk()) { }
}

void loop() {
    int s = digitalRead(TRIGGER_PIN);

    if (s == LOW && lastState == HIGH) {
        captureFast();
    }

    lastState = s;
}

inline void captureFast() {
    digitalWrite(LED_PIN, HIGH);

    if (!camera.capture().isOk()) {
        digitalWrite(LED_PIN, LOW);
        return;
    }

    char fname[32];
    imgCounter++;
    snprintf(fname, sizeof(fname), "/i%lu.jpg", imgCounter);

    sdmmc.save(camera.frame).to(fname, "jpg");

    digitalWrite(LED_PIN, LOW);
}
