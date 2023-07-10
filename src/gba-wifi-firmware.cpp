#include <LittleFS.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#include "gba-spi.h"
#include "multiboot.h"

// Create a secrets.h file with #define's for WIFI_SSID and WIFI_PASS to hardcode your WiFi credentials.
// This will lead to a faster and more stable connection than the configuration portal.
// Alternatively, comment out this line to use the portal.
#include "secrets.h"

ESP8266WebServer server(80);
WiFiManager wifiManager;
Multiboot multiboot;

void handleRoot();
void handleUpload();


#define GBA_WIFI_CONSTANT_ROM_LOADED 0x0310FEED


void setup() {
  Serial.begin(115200);
  Serial.setTimeout(-1);

  Serial.println("Starting...");

  GBA_SPI.begin();

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

  Serial.println("Server started.");
}

void loop() {
  uint32_t r = GBA_SPI.WriteSPI32(multiboot.MULTIBOOT_START_COMMAND);
  if (r >> 16 == multiboot.MULTIBOOT_START_RESPONSE) {
    multiboot.startMultiboot(LittleFS.open("/rom_mb.gba", "r"));
    delay(1000);
  }
  if (r == GBA_WIFI_CONSTANT_ROM_LOADED) {
    for (uint32_t x = 93; x < 120; x++) {
      Serial.print("Next: ");

      uint32_t response = GBA_SPI.WriteSPI32(x);
      Serial.println(response);

      delay(100);
    }
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
        GBA_SPI.WriteSPI32(0xAABBCCDD);
      }
    }
    yield();
  });
}
