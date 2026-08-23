#include <Arduino.h>

#include "DisplayManager.h"
#include "WiFiProvisioning.h"
#include "config.h"

namespace {
DisplayManager display;
WiFiProvisioning wifi;
}  // namespace

void setup() {
  Serial.begin(Config::SERIAL_BAUD_RATE);
  delay(200);

  Serial.println();
  Serial.printf("[BOOT] %s firmware starting\n", Config::PRODUCT_NAME);
  Serial.printf("[BOOT] Firmware version: %s\n", Config::FIRMWARE_VERSION);

  display.begin();
  display.showSplash();
  delay(Config::SPLASH_DURATION_MS);

  if (!wifi.connect(display)) {
    Serial.println("[WIFI] Restarting after provisioning failure");
    delay(3000);
    ESP.restart();
  }

  display.showConnected(Config::FIRMWARE_VERSION, WiFi.localIP());
  Serial.println("[APP] Main screen loaded");
}

void loop() {
  delay(1000);
}
