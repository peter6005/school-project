#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LoRa.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_SGP30.h>
#include <DHT.h>
#include "pico/multicore.h"
#include "pico/cyw43_arch.h"

#define SEA_LEVEL_PRESSURE 101325.0f

// I2C
#define I2C_SDA 4
#define I2C_SCL 5

// SD SPI1
#define SD_MISO 12
#define SD_CS   13
#define SD_SCK  14
#define SD_MOSI 15

// LoRa SPI0
#define LORA_MISO 16
#define LORA_CS   17
#define LORA_SCK  18
#define LORA_MOSI 19
#define LORA_RST  21

// ESP32-CAM enable (PNP -> enable pin)
#define CAM_ENABLE_PIN 27

// DHT
#define DHT_PIN 28
#define DHT_TYPE DHT11

Adafruit_BMP280 bmp;
Adafruit_SGP30 sgp;
DHT dht(DHT_PIN, DHT_TYPE);
File logFile;

bool bmp_ok = false;
bool sgp_ok = false;
bool dht_ok = false;
bool sd_ok  = false;
bool lora_ok = false;

char line_buffer[128];
volatile bool line_ready = false;

// ---------- BASIC XOR CHECKSUM ----------
uint8_t simple_checksum(const char *data) {
  uint8_t cs = 0;
  while (*data) {
    cs ^= (uint8_t)(*data++);
  }
  return cs;
}

// ---------- CORE1 LoRa ----------
void core1_lora_task() {

  SPI.setRX(LORA_MISO);
  SPI.setTX(LORA_MOSI);
  SPI.setSCK(LORA_SCK);
  SPI.begin();

  LoRa.setSPI(SPI);
  LoRa.setPins(LORA_CS, LORA_RST, -1);

  if (!LoRa.begin(868E6)) {
    lora_ok = false;
    while (1) delay(100);
  }

  LoRa.setTxPower(17);
  lora_ok = true;

  while (true) {
    multicore_fifo_pop_blocking();

    if (!line_ready || !lora_ok) continue;

    LoRa.beginPacket();
    LoRa.print(line_buffer);
    LoRa.endPacket();

    line_ready = false;
  }
}

// ---------- SETUP ----------
void setup() {

  Serial.begin(115200);
  delay(500);

  // kill onboard radio ASAP
  cyw43_arch_deinit();

  pinMode(CAM_ENABLE_PIN, OUTPUT);
  digitalWrite(CAM_ENABLE_PIN, HIGH);

  Wire.setSDA(I2C_SDA);
  Wire.setSCL(I2C_SCL);
  Wire.begin();

  bmp_ok = bmp.begin(0x76);

  sgp_ok = sgp.begin();
  if (sgp_ok) {
    for (int i = 0; i < 15; i++) {
      sgp.IAQmeasure();
      delay(1000);
    }
  }

  dht.begin();
  delay(200);
  dht_ok = !isnan(dht.readTemperature());

  SPI1.setRX(SD_MISO);
  SPI1.setTX(SD_MOSI);
  SPI1.setSCK(SD_SCK);
  SPI1.begin();
  sd_ok = SD.begin(SD_CS, SPI1);

  multicore_launch_core1(core1_lora_task);
}

// ---------- LOOP ----------
void loop() {

  char buf[128];
  buf[0] = 0;

  // ---------- BMP280 ----------
  if (bmp_ok) {
    int32_t t = (int32_t)(bmp.readTemperature() * 100);
    int32_t p = (int32_t)(bmp.readPressure());

    float altitude_f = 44330.0f * (1.0f - powf((float)p / SEA_LEVEL_PRESSURE, 0.1903f));
    int32_t alt = (int32_t)(altitude_f * 100);

    if (t > -4000 && t < 8500 && p > 30000 && p < 120000) {
      char tmp[48];
      snprintf(tmp, sizeof(tmp), "%ld,%ld,%ld,", t, p, alt);
      strcat(buf, tmp);
    } else {
      bmp_ok = false;
      strcat(buf, ",,,");
    }
  } else {
    strcat(buf, ",,,");
  }

  // ---------- SGP30 ----------
  if (sgp_ok && sgp.IAQmeasure()) {
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "%u,%u,", sgp.eCO2, sgp.TVOC);
    strcat(buf, tmp);
  } else {
    strcat(buf, ",,");
  }

  // ---------- DHT ----------
  if (dht_ok) {
    int32_t dt = (int32_t)(dht.readTemperature() * 100);
    int32_t dh = (int32_t)(dht.readHumidity() * 100);

    if (dt > -4000 && dt < 8500 && dh >= 0 && dh <= 10000) {
      char tmp[32];
      snprintf(tmp, sizeof(tmp), "%ld,%ld", dt, dh);
      strcat(buf, tmp);
    } else {
      strcat(buf, ",");
    }
  } else {
    strcat(buf, ",");
  }

  // ---------- CHECKSUM ----------
  uint8_t cs = simple_checksum(buf);
  char cs_str[8];
  snprintf(cs_str, sizeof(cs_str), ",%02X\n", cs);
  strcat(buf, cs_str);

  // ---------- SD ----------
  if (sd_ok) {
    logFile = SD.open("/log.txt", FILE_WRITE);
    if (logFile) {
      logFile.print(buf);
      logFile.close();
    }
  }

  // ---------- LORA ----------
  if (lora_ok) {
    strcpy(line_buffer, buf);
    line_ready = true;
    multicore_fifo_push_blocking(1);
  }

  delay(900);
}
