#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

// LoRa - SPI0
#define LORA_MISO 16
#define LORA_CS   17
#define LORA_SCK  18
#define LORA_MOSI 19
#define LORA_RST  21

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\nLoRa RX BOOT");

  SPI.setRX(LORA_MISO);
  SPI.setTX(LORA_MOSI);
  SPI.setSCK(LORA_SCK);
  SPI.begin();

  LoRa.setSPI(SPI);
  LoRa.setPins(LORA_CS, LORA_RST, -1);

  if (!LoRa.begin(868E6)) {
    Serial.println("LoRa INIT FAILED");
    while (1) delay(100);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.enableCrc();

  Serial.println("LoRa RX READY");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.print("RX packet: ");

    String payload = "";
    while (LoRa.available()) {
      payload += (char)LoRa.read();
    }

    int rssi = LoRa.packetRssi();

    Serial.println(payload);
    Serial.print("RSSI: ");
    Serial.print(rssi);
    Serial.println(" dBm\n");
  }
}
