#include "multiboot.h"

bool Multiboot::isGbaReady() {
    if (GBA_SPI.WriteSPI32(MULTIBOOT_START_COMMAND) >> 16 == MULTIBOOT_START_RESPONSE) {
        return true;
    }
    return false;
}

void Multiboot::startMultiboot(File romfile) {
  uint32_t fpos;
  uint32_t r, w, bit, retry_count;

  if (!romfile) {
    Serial.println("Failed to open file for reading");
    return;
  }

  uint32_t fsize = (romfile.size() + 0x0f) & 0xfffffff0;

  Serial.print("ROM size: "); Serial.println(fsize);

  if (fsize > 0x40000) {
    Serial.println("ROM file too large!");
    return;
  }

  Serial.println("Searching for GBA");
  while(GBA_SPI.WriteSPI32(MULTIBOOT_START_COMMAND) >> 16 != MULTIBOOT_START_RESPONSE) { // GBA SP returns 0x72026203?
    yield(); // keep ESP8266 WDT happy
  }

  Serial.println("GBA found!");
  r = GBA_SPI.WriteSPI32(MULTIBOOT_START_COMMAND);
  Serial.println("Recognition OK");
  r = GBA_SPI.WriteSPI32(0x00006102);

  Serial.println("Transferring header");
  for (fpos = 0; fpos < 0xC0; fpos += 2) {
    r = GBA_SPI.WriteSPI32(0x0000FFFF & (romfile.read() | (romfile.read() << 8)));
  }

  Serial.println("Header transfer complete");
  r = GBA_SPI.WriteSPI32(0x00006200);
  Serial.println("Exchange master/slave information");
  r = GBA_SPI.WriteSPI32(0x00006202);

  Serial.println("Sending palette data");
  r = GBA_SPI.WriteSPI32(0x000063d1);
  Serial.println("Sending palette data, receive 0x73hh****");
  r = GBA_SPI.WriteSPI32(0x000063d1);

  uint32_t m = ((r & 0x00ff0000) >>  8) + 0xffff00d1;
  uint32_t h = ((r & 0x00ff0000) >> 16) + 0xf;

  Serial.println("Sending handshake data");
  r = GBA_SPI.WriteSPI32((((r >> 16) + 0xf) & 0xff) | 0x00006400);
  Serial.println("Sending length info, receive seed 0x**cc****");
  r = GBA_SPI.WriteSPI32((fsize - 0xC0) / 4 - 0x34);

  uint32_t f = (((r & 0x00ff0000) >> 8) + h) | 0xffff0000;
  uint32_t c = 0x0000c387;

  unsigned long loopStart = 0;
  unsigned long step1Time = 0;
  unsigned long step2Time = 0;
  unsigned long step3Time = 0;
  unsigned long step4Time = 0;

  Serial.println("Sending encrypted ROM data");
  for(; fpos < fsize; fpos += 4) {
    loopStart = millis();

    w = (uint32_t)romfile.read() | (uint32_t)(romfile.read() << 8) | (uint32_t)(romfile.read() << 16) | (uint32_t)(romfile.read() << 24);

    step1Time += (millis() - loopStart);
    loopStart = millis();

    if (fpos % (fsize / 32) == 0) {
      Serial.print("#");
    }

    m = (0x6f646573 * m) + 1;

    // transfer data (32-bits) to GBA
    GBA_SPI.WriteSPI32(w ^ ((~(0x02000000 + fpos)) + 1) ^ m ^ 0x43202f2f);

    step2Time += (millis() - loopStart);
    loopStart = millis();

    for (bit = 0; bit < 32; bit++) {
      if ((c ^ w) & 0x01) {
        c = (c >> 1) ^ 0x0000c37b;
      } else {
        c = c >> 1;
      }
      w = w >> 1;
    }

    step3Time += (millis() - loopStart);
    loopStart = millis();

    yield(); // keep ESP8266 WDT happy

    step4Time += (millis() - loopStart);
  }

  Serial.print("Times: ");
  Serial.print(" ");
  Serial.print(step1Time);
  Serial.print(" ");
  Serial.print(step2Time);
  Serial.print(" ");
  Serial.print(step3Time);
  Serial.print(" ");
  Serial.println(step4Time);

  Serial.println("\nROM sent! Calculating checksum");
  for (bit = 0; bit < 32; bit++) {
    if ((c ^ f) & 0x01) {
      c = ( c >> 1) ^ 0x0000c37b;
    } else {
      c = c >> 1;
    }
    f = f >> 1;
  }

  Serial.print("Waiting for GBA to respond with CRC: ");
  retry_count = 10000;
  while(GBA_SPI.WriteSPI32(0x00000065) != 0x00750065 && (--retry_count) > 0) {
    yield(); // keep ESP8266 WDT happy
  }
  if (retry_count == 0) {
    Serial.println("Timeout waiting for CRC.");
    return;
  }

  Serial.println("GBA CRC ready");
  r = GBA_SPI.WriteSPI32(0x00000066);
  Serial.print("Exchanging CRC's - ");
  r = GBA_SPI.WriteSPI32(c);
  Serial.print("Expected CRC: "); Serial.print(c, HEX); Serial.print(", GBA CRC: "); Serial.println(r >> 16, HEX);

  if(r >> 16 == c) {
    Serial.println("CRC's match! Executing ROM");
  } else {
    Serial.println("CRC's mismatch! Something went wrong");
  }
}

Multiboot::Multiboot() {
}

Multiboot::~Multiboot() {
}