#include <Arduino.h>

#include "DisplayManager.h"
#include "OtaManager.h"
#include "WiFiProvisioning.h"
#include "config.h"

namespace {
DisplayManager display;
WiFiProvisioning wifi;
OtaManager ota;
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

  if (!ota.isConfigured()) {
    Serial.println("[OTA] Disabled: include/ota_secrets.h is not configured");
    return;
  }

  ota.reportPendingInstallation();
  Serial.println("[OTA] Checking for firmware updates");
  bool updateScreenShown = false;
  const OtaManager::Result otaResult = ota.checkAndInstall(
      [&updateScreenShown](size_t received, size_t total) {
        if (!updateScreenShown) {
          display.showOtaStatus("Updating", "Downloading firmware");
          updateScreenShown = true;
        }
        display.showOtaProgress(received, total);
      });

  if (otaResult == OtaManager::Result::NoUpdate) {
    Serial.println("[OTA] Device firmware is current");
    display.showConnected(Config::FIRMWARE_VERSION, WiFi.localIP());
  } else if (otaResult == OtaManager::Result::Installed) {
    Serial.printf("[OTA] Version %s staged; restarting\n",
                  ota.availableVersion().c_str());
    display.showOtaStatus("Update Ready", "Restarting...");
    delay(1500);
    ESP.restart();
  } else if (otaResult == OtaManager::Result::Failed) {
    Serial.printf("[OTA] Update failed: %s\n", ota.lastError().c_str());
    display.showConnected(Config::FIRMWARE_VERSION, WiFi.localIP());
  }
}

void loop() {
  delay(1000);
}
