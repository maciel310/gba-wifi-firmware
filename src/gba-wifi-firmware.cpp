#include <LittleFS.h>
#include <SPI.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

ESP8266WebServer server(80);
WiFiManager wifiManager;

// Uncomment the following lines to hardcode your WiFi credentials. This will lead to a faster and more stable
// connection than the configuration portal.
// #define WIFI_SSID "YOUR_SSID"
// #define WIFI_PASS "YOUR_PASSWORD"

void handleRoot();
void handleUpload();
uint32_t WriteSPI32(uint32_t w);
void upload(void);


void setup() {
  Serial.begin(115200);
  Serial.setTimeout(-1);

  Serial.println("Starting...");

  SPI.setDataMode(SPI_MODE3);
  SPI.setBitOrder(MSBFIRST);
  SPI.setFrequency(500000);
  SPI.begin();

  LittleFS.begin();

  Serial.println("Connecting to WiFi");

  #ifdef WIFI_SSID
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  #else
    wifiManager.setConfigPortalBlocking(false);
    wifiManager.autoConnect("GbaWiFi");
  #endif

  if (MDNS.begin("gba")) {
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  server.on("/", handleRoot);
  handleUpload();
  server.begin();
  MDNS.addService("http", "tcp", 80);

  Serial.println("Server connected.");
}

void loop() {
  if (WriteSPI32(0x00006202) >> 16 == 0x7202) {
    upload();
  }

  wifiManager.process();
  server.handleClient();
  MDNS.update();

  delay(100);
}

void handleRoot() {
  server.send(200, "text/html", "<form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='name'><button type='submit'>Upload</button>");
}

File romfile;
void handleUpload() {
  server.on("/upload", HTTP_POST, []() {
    Serial.println("/upload requested");
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", "Done!");
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.println("Upload start");
      romfile = LittleFS.open("/rom_mb.gba", "w+");
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      Serial.println("Upload write");
      romfile.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
      Serial.println("Upload complete");
      romfile.close();

      // Send REBOOT signal to running ROM.
      for (int i = 0; i < 10; i++) {
        Serial.println("Sending reboot signal");
        WriteSPI32(0xAABBCCDD);
      }
    }
    yield();
  });
}

uint32_t WriteSPI32(uint32_t w) {
  uint32_t rx[4];

  rx[0] = SPI.transfer((w >> 24) & 0xFF);
  rx[1] = SPI.transfer((w >> 16) & 0xFF);
  rx[2] = SPI.transfer((w >> 8) & 0xFF);
  rx[3] = SPI.transfer(w & 0xFF);

  delayMicroseconds(10);

  return rx[3] | (rx[2] << 8) | (rx[1] << 16) | (rx[0] << 24);
}

void upload(void) {
  long fpos;
  uint32_t r, w, bit;

  File romfile = LittleFS.open("/rom_mb.gba", "r");
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
  while(WriteSPI32(0x00006202) >> 16 != 0x7202) { // GBA SP returns 0x72026203?
    yield(); // keep ESP8266 WDT happy
  }

  Serial.println("GBA found!");
  r = WriteSPI32(0x00006202);
  Serial.println("Recognition OK");
  r = WriteSPI32(0x00006102);

  Serial.println("Transferring header");
  for (fpos = 0; fpos < 0xC0; fpos += 2) {
    r = WriteSPI32(0x0000FFFF & (romfile.read() | (romfile.read() << 8)));
  }

  Serial.println("Header transfer complete");
  r = WriteSPI32(0x00006200);
  Serial.println("Exchange master/slave information");
  r = WriteSPI32(0x00006202);

  Serial.println("Sending palette data");
  r = WriteSPI32(0x000063d1);
  Serial.println("Sending palette data, receive 0x73hh****");
  r = WriteSPI32(0x000063d1);

  uint32_t m = ((r & 0x00ff0000) >>  8) + 0xffff00d1;
  uint32_t h = ((r & 0x00ff0000) >> 16) + 0xf;

  Serial.println("Sending handshake data");
  r = WriteSPI32((((r >> 16) + 0xf) & 0xff) | 0x00006400);
  Serial.println("Sending length info, receive seed 0x**cc****");
  r = WriteSPI32((fsize - 0xC0) / 4 - 0x34);

  uint32_t f = (((r & 0x00ff0000) >> 8) + h) | 0xffff0000;
  uint32_t c = 0x0000c387;

  unsigned long loopStart = 0;
  unsigned long step1Time = 0;
  unsigned long step2Time = 0;
  unsigned long step3Time = 0;
  unsigned long step4Time = 0;

  Serial.println("Sending encrypted ROM data");
  for(fpos; fpos < fsize; fpos += 4) {
    loopStart = millis();

    w = (uint32_t)romfile.read() | (uint32_t)(romfile.read() << 8) | (uint32_t)(romfile.read() << 16) | (uint32_t)(romfile.read() << 24);

    step1Time += (millis() - loopStart);
    loopStart = millis();

    if (fpos % (fsize / 32) == 0) {
      Serial.print("#");
    }

    m = (0x6f646573 * m) + 1;

    // transfer data (32-bits) to GBA
    WriteSPI32(w ^ ((~(0x02000000 + fpos)) + 1) ^ m ^ 0x43202f2f);

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
  while(WriteSPI32(0x00000065) != 0x00750065) {
    yield(); // keep ESP8266 WDT happy
  }

  Serial.println("GBA CRC ready");
  r = WriteSPI32(0x00000066);
  Serial.print("Exchanging CRC's - ");
  r = WriteSPI32(c);
  Serial.print("Expected CRC: "); Serial.print(c, HEX); Serial.print(", GBA CRC: "); Serial.println(r >> 16, HEX);

  if(r >> 16 == c) {
    Serial.println("CRC's match! Executing ROM");
  } else {
    Serial.println("CRC's mismatch! Something went wrong");
  }
}
