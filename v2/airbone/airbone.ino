#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LoRa.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_SGP30.h>
#include <DHT.h>

#include "pico/multicore.h"

#define I2C_SDA 4
#define I2C_SCL 5

#define DHT_PIN 28
#define DHT_TYPE DHT11

// SD SPI1
#define SD_MISO 12
#define SD_CS 13
#define SD_SCK 14
#define SD_MOSI 15

// LoRa SPI0
#define LORA_MISO 16
#define LORA_CS 17
#define LORA_SCK 18
#define LORA_MOSI 19
#define LORA_RST 21

#define TRIGGER_OUT_PIN 27
#define TRIGGER_PULSE_MS 150

Adafruit_BMP280 bmp;
Adafruit_SGP30 sgp;
DHT dht(DHT_PIN, DHT_TYPE);
File logFile;

bool bmp_ok = false;
bool sgp_ok = false;
bool dht_ok = false;
bool sd_ok = false;
bool lora_ok = false;

char line_buffer[96];
volatile bool line_ready = false;

void core1_lora_task()
{

  SPI.setRX(LORA_MISO);
  SPI.setTX(LORA_MOSI);
  SPI.setSCK(LORA_SCK);
  SPI.begin();

  LoRa.setSPI(SPI);
  LoRa.setPins(LORA_CS, LORA_RST, -1);

  if (!LoRa.begin(868E6))
  {
    lora_ok = false;
    while (1)
      delay(100);
  }

  LoRa.setTxPower(17);
  lora_ok = true;

  while (true)
  {
    multicore_fifo_pop_blocking();

    if (!line_ready || !lora_ok)
      continue;

    bool has_data = false;
    for (uint8_t i = 0; i < strlen(line_buffer); i++)
    {
      if (line_buffer[i] != ',' && line_buffer[i] != '\n')
      {
        has_data = true;
        break;
      }
    }
    if (!has_data)
    {
      line_ready = false;
      continue;
    }

    LoRa.beginPacket();
    LoRa.print(line_buffer);
    LoRa.endPacket();

    line_ready = false;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  pinMode(TRIGGER_OUT_PIN, OUTPUT);
  digitalWrite(TRIGGER_OUT_PIN, HIGH);

  Wire.setSDA(I2C_SDA);
  Wire.setSCL(I2C_SCL);
  Wire.begin();

  // BMP280
  bmp_ok = bmp.begin(0x76);

  // SGP30
  sgp_ok = sgp.begin();
  if (sgp_ok)
  {
    for (int i = 0; i < 15; i++)
    {
      sgp.IAQmeasure();
      delay(1000);
    }
  }

  // DHT
  dht.begin();
  delay(200);
  dht_ok = !isnan(dht.readTemperature());

  // SD
  SPI1.setRX(SD_MISO);
  SPI1.setTX(SD_MOSI);
  SPI1.setSCK(SD_SCK);
  SPI1.begin();
  sd_ok = SD.begin(SD_CS, SPI1);

  multicore_launch_core1(core1_lora_task);
}

void loop()
{

  char buf[96];
  buf[0] = 0;

  // ---------- BMP280 ----------
  if (bmp_ok)
  {
    int32_t t = (int32_t)(bmp.readTemperature() * 100);
    int32_t p = (int32_t)(bmp.readPressure() * 100 / 100.0);

    // validate
    if (t > -4000 && t < 8500 && p > 30000 && p < 120000)
    {
      char tmp[24];
      snprintf(tmp, sizeof(tmp), "%ld,%ld,", t, p);
      strcat(buf, tmp);
    }
    else
    {
      bmp_ok = false;
      strcat(buf, ",,");
    }
  }
  else
  {
    strcat(buf, ",,");
  }

  // ---------- SGP30 ----------
  if (sgp_ok)
  {
    if (sgp.IAQmeasure())
    {
      char tmp[24];
      snprintf(tmp, sizeof(tmp), "%u,%u,", sgp.eCO2, sgp.TVOC);
      strcat(buf, tmp);
    }
    else
    {
      sgp_ok = false;
      strcat(buf, ",,");
    }
  }
  else
  {
    strcat(buf, ",,");
  }

  // ---------- DHT ----------
  if (dht_ok)
  {
    int32_t dt = (int32_t)(dht.readTemperature() * 100);
    int32_t dh = (int32_t)(dht.readHumidity() * 100);

    if (dt > -4000 && dt < 8500 && dh >= 0 && dh <= 10000)
    {
      char tmp[24];
      snprintf(tmp, sizeof(tmp), "%ld,%ld", dt, dh);
      strcat(buf, tmp);
    }
    else
    {
      dht_ok = false;
      strcat(buf, ",");
    }
  }
  else
  {
    strcat(buf, ",");
  }

  strcat(buf, "\n");

  // ---------- SD ----------
  if (sd_ok)
  {
    logFile = SD.open("/log.txt", FILE_WRITE);
    if (logFile)
    {
      logFile.print(buf);
      logFile.close();
    }
  }

  // ---------- ESP32-CAM trigger ----------
  digitalWrite(TRIGGER_OUT_PIN, LOW);
  delay(TRIGGER_PULSE_MS);
  digitalWrite(TRIGGER_OUT_PIN, HIGH);

  // ---------- send to core1 ----------
  if (lora_ok && (bmp_ok || sgp_ok || dht_ok))
  {
    strcpy(line_buffer, buf);
    line_ready = true;
    multicore_fifo_push_blocking(1);
  }

  delay(724);
}
