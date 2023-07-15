#include "gba-spi.h"

GbaSpi GBA_SPI;

GbaSpi::GbaSpi() {
}

void GbaSpi::begin() {
    SPI.setDataMode(SPI_MODE3);
    SPI.setBitOrder(MSBFIRST);
    SPI.setFrequency(256000);
    SPI.begin();
}

uint32_t GbaSpi::WriteSPI32(uint32_t w) {
  if (debugging_enabled) {
    Serial.print("Sending: ");
    Serial.print(w, HEX);
  }
  uint32_t rx[4];

  rx[0] = SPI.transfer((w >> 24) & 0xFF);
  rx[1] = SPI.transfer((w >> 16) & 0xFF);
  rx[2] = SPI.transfer((w >> 8) & 0xFF);
  rx[3] = SPI.transfer(w & 0xFF);

  delayMicroseconds(10);

  uint32_t response = rx[3] | (rx[2] << 8) | (rx[1] << 16) | (rx[0] << 24);
  if (debugging_enabled) {
    Serial.print(" Received: ");
    Serial.println(response, HEX);
  }
  return response;
}

GbaSpi::~GbaSpi() {
    SPI.end();
}

void GbaSpi::enableDebugging() {
  debugging_enabled = true;
}

void GbaSpi::disableDebugging() {
  debugging_enabled = false;
}