#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

#define LORA_MISO 16
#define LORA_CS   17
#define LORA_SCK  18
#define LORA_MOSI 19
#define LORA_RST  21

uint8_t simple_checksum(const char *data) {
  uint8_t cs = 0;
  while (*data) {
    cs ^= (uint8_t)(*data++);
  }
  return cs;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  SPI.setRX(LORA_MISO);
  SPI.setTX(LORA_MOSI);
  SPI.setSCK(LORA_SCK);
  SPI.begin();

  LoRa.setSPI(SPI);
  LoRa.setPins(LORA_CS, LORA_RST, -1);

  if (!LoRa.begin(868E6)) {
    while (1);
  }

  Serial.println("LoRa RX ready");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";

    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    received.trim();

    int lastComma = received.lastIndexOf(',');
    if (lastComma > 0) {
      String payload = received.substring(0, lastComma);
      String cs_str  = received.substring(lastComma + 1);

      uint8_t calc = simple_checksum(payload.c_str());
      uint8_t recv = strtol(cs_str.c_str(), NULL, 16);

      if (calc == recv) {
        Serial.println(payload);
      }
    }
  }
}
