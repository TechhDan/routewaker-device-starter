#include <Arduino.h>
#include <time.h>

#include "DisplayManager.h"
#include "DeviceIdentity.h"
#include "OtaManager.h"
#include "WiFiProvisioning.h"
#include "config.h"

namespace {
DisplayManager display;
WiFiProvisioning wifi;
OtaManager ota;
String hardwareId;
String provisioningUrl;
bool provisioningQrVisible = false;
bool lastRawButtonState = HIGH;
bool stableButtonState = HIGH;
unsigned long buttonChangedAt = 0;

bool otaNeedsTrustedClock() {
  return String(Config::OTA_API_BASE_URL).startsWith("https://") &&
         strlen(Config::OTA_ROOT_CA) > 0;
}

bool synchronizeClock() {
  Serial.println("[TIME] Synchronizing clock for TLS validation");
  configTime(0, 0, Config::NTP_PRIMARY_SERVER, Config::NTP_FALLBACK_SERVER);

  const unsigned long startedAt = millis();
  time_t now = time(nullptr);
  while (now < 1704067200 &&
         millis() - startedAt < Config::NTP_SYNC_TIMEOUT_MS) {
    delay(250);
    now = time(nullptr);
  }

  if (now < 1704067200) {
    Serial.println("[TIME] Synchronization failed; secure OTA is unavailable");
    return false;
  }

  struct tm utcTime;
  gmtime_r(&now, &utcTime);
  char timestamp[25];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &utcTime);
  Serial.printf("[TIME] Clock synchronized: %s\n", timestamp);
  return true;
}
}  // namespace

void setup() {
  Serial.begin(Config::SERIAL_BAUD_RATE);
  delay(200);

  Serial.println();
  Serial.printf("[BOOT] %s firmware starting\n", Config::PRODUCT_NAME);
  Serial.printf("[BOOT] Firmware version: %s\n", Config::FIRMWARE_VERSION);
  hardwareId = DeviceIdentity::hardwareId();
  provisioningUrl = DeviceIdentity::provisioningUrl();
  Serial.printf("[DEVICE] Hardware ID: %s\n", hardwareId.c_str());
  Serial.println("[DEVICE] Press BOOT to show the provisioning QR code");

  pinMode(Config::DEVICE_INFO_BUTTON_PIN, INPUT_PULLUP);

  display.begin();
  display.showSplash(hardwareId);
  delay(Config::SPLASH_DURATION_MS);

  if (!wifi.connect(display)) {
    Serial.println("[WIFI] Restarting after provisioning failure");
    delay(3000);
    ESP.restart();
  }

  display.showConnected(Config::FIRMWARE_VERSION, WiFi.localIP(), hardwareId);
  Serial.println("[APP] Main screen loaded");

  if (!ota.isConfigured()) {
    Serial.println("[OTA] Disabled: include/ota_secrets.h is not configured");
    return;
  }

  if (otaNeedsTrustedClock() && !synchronizeClock()) {
    Serial.println("[OTA] Skipped because the TLS certificate cannot be validated");
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
    display.showConnected(Config::FIRMWARE_VERSION, WiFi.localIP(), hardwareId);
  } else if (otaResult == OtaManager::Result::Installed) {
    Serial.printf("[OTA] Version %s staged; restarting\n",
                  ota.availableVersion().c_str());
    display.showOtaStatus("Update Ready", "Restarting...");
    delay(1500);
    ESP.restart();
  } else if (otaResult == OtaManager::Result::Failed) {
    Serial.printf("[OTA] Update failed: %s\n", ota.lastError().c_str());
    display.showConnected(Config::FIRMWARE_VERSION, WiFi.localIP(), hardwareId);
  }
}

void loop() {
  const bool rawButtonState =
      digitalRead(Config::DEVICE_INFO_BUTTON_PIN);
  if (rawButtonState != lastRawButtonState) {
    lastRawButtonState = rawButtonState;
    buttonChangedAt = millis();
  }

  if (millis() - buttonChangedAt >= Config::BUTTON_DEBOUNCE_MS &&
      stableButtonState != rawButtonState) {
    stableButtonState = rawButtonState;
    if (stableButtonState == LOW) {
      provisioningQrVisible = !provisioningQrVisible;
      if (provisioningQrVisible) {
        display.showDeviceProvisioningQr(provisioningUrl, hardwareId);
      } else {
        display.showConnected(Config::FIRMWARE_VERSION, WiFi.localIP(),
                              hardwareId);
      }
    }
  }

  delay(10);
}
