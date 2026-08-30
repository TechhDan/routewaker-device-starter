#include <Arduino.h>
#include <time.h>
#include <WiFi.h>
#include <esp_ota_ops.h>
#include <esp_system.h>

#include "DeviceConfiguration.h"
#include "DisplayManager.h"
#include "DeviceIdentity.h"
#include "OtaManager.h"
#include "ProvisioningHandoff.h"
#include "WiFiProvisioning.h"
#include "config.h"

namespace {
DisplayManager display;
WiFiProvisioning wifi;
OtaManager ota;
String hardwareId;
constexpr unsigned long PendingConfirmationRetryMs = 45000;
bool pendingConfirmationRetry = false;
bool normalOtaLifecycleComplete = false;
unsigned long nextPendingConfirmationAttempt = 0;

bool otaNeedsTrustedClock(const String& platformUrl) {
  return platformUrl.startsWith("https://") &&
         strlen(Config::OTA_ROOT_CA) > 0;
}

String eventUuid() {
  uint8_t bytes[16];
  esp_fill_random(bytes, sizeof(bytes));
  bytes[6] = (bytes[6] & 0x0F) | 0x40;
  bytes[8] = (bytes[8] & 0x3F) | 0x80;
  char output[37];
  snprintf(output, sizeof(output),
           "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-"
           "%02x%02x%02x%02x%02x%02x",
           bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5],
           bytes[6], bytes[7], bytes[8], bytes[9], bytes[10], bytes[11],
           bytes[12], bytes[13], bytes[14], bytes[15]);
  return String(output);
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

bool connectInstallerWiFi(const ProvisioningHandoff& handoff) {
  Serial.printf("[HANDOFF] Connecting to installer Wi-Fi: %s\n",
                handoff.ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(handoff.ssid.c_str(), handoff.password.c_str());
  const unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedAt < Config::WIFI_CONNECT_TIMEOUT_SECONDS * 1000UL) {
    delay(250);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HANDOFF] Installer Wi-Fi connection failed; handoff retained");
    return false;
  }
  Serial.printf("[HANDOFF] Connected with IP %s\n",
                WiFi.localIP().toString().c_str());
  return true;
}

bool completeProvisionerHandoff(ProvisioningHandoff& handoff) {
  if (handoff.expectedVersion != Config::FIRMWARE_VERSION) {
    Serial.printf("[HANDOFF] Expected firmware %s, but running %s; handoff retained\n",
                  handoff.expectedVersion.c_str(), Config::FIRMWARE_VERSION);
    return false;
  }
  if (!connectInstallerWiFi(handoff)) return false;
  if (otaNeedsTrustedClock(handoff.platformUrl) && !synchronizeClock()) return false;
  if (!DeviceConfiguration::save(handoff.platformUrl, handoff.deviceToken)) {
    Serial.println("[HANDOFF] Permanent device identity could not be stored");
    return false;
  }
  ota.configure(handoff.platformUrl, handoff.deviceToken);
  if (handoff.eventId.isEmpty()) {
    handoff.eventId = eventUuid();
    if (!ProvisioningHandoff::saveEventId(handoff.eventId)) {
      Serial.println("[HANDOFF] Stable installation event ID could not be stored");
      return false;
    }
  }
  if (!ota.reportInstallation(handoff.releaseId, handoff.eventId)) {
    Serial.println("[HANDOFF] Installation confirmation failed; handoff retained");
    return false;
  }
  if (!ota.sendHeartbeat()) {
    Serial.printf("[HANDOFF] Heartbeat failed: %s; handoff retained\n",
                  ota.lastError().c_str());
    return false;
  }

  const esp_err_t validation = esp_ota_mark_app_valid_cancel_rollback();
  if (validation != ESP_OK && validation != ESP_ERR_NOT_SUPPORTED) {
    Serial.printf("[HANDOFF] Could not mark application valid: %d\n", validation);
    return false;
  }
  if (!WiFi.disconnect(true, true)) {
    Serial.println("[HANDOFF] SDK installer Wi-Fi credentials could not be erased");
    return false;
  }
  if (!ProvisioningHandoff::clearBootstrap() || !ProvisioningHandoff::clear()) {
    Serial.println("[HANDOFF] Provisioning namespaces could not be cleared");
    return false;
  }
  delay(250);
  Serial.println("[HANDOFF] Provisioner data and installer Wi-Fi erased");
  return true;
}

void runNormalOtaLifecycle() {
  if (normalOtaLifecycleComplete) return;
  normalOtaLifecycleComplete = true;

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

void attemptPendingInstallationConfirmation() {
  if (ota.reportPendingInstallation()) {
    pendingConfirmationRetry = false;
    display.showConnected(Config::FIRMWARE_VERSION, WiFi.localIP(), hardwareId);
    runNormalOtaLifecycle();
    return;
  }

  pendingConfirmationRetry = true;
  nextPendingConfirmationAttempt = millis() + PendingConfirmationRetryMs;
  Serial.printf("[OTA] Installation confirmation pending: %s Retrying in %lu seconds.\n",
                ota.lastError().c_str(), PendingConfirmationRetryMs / 1000UL);
  display.showOtaStatus("Confirmation Pending", "Will retry automatically");
}
}  // namespace

void setup() {
  Serial.begin(Config::SERIAL_BAUD_RATE);
  delay(200);

  Serial.println();
  Serial.printf("[BOOT] %s firmware starting\n", Config::PRODUCT_NAME);
  Serial.printf("[BOOT] Firmware version: %s\n", Config::FIRMWARE_VERSION);
  hardwareId = DeviceIdentity::hardwareId();
  Serial.printf("[DEVICE] Hardware ID: %s\n", hardwareId.c_str());

  display.begin();
  display.showSplash(hardwareId);
  delay(Config::SPLASH_DURATION_MS);

  ProvisioningHandoff handoff;
  const bool handoffPending = ProvisioningHandoff::isPending();
  const bool handoffValid = ProvisioningHandoff::load(handoff);
  if (handoffPending && !handoffValid) {
    Serial.println("[HANDOFF] Pending handoff is incomplete or invalid; onboarding blocked");
    display.showOtaStatus("Activation Failed", "Invalid installer handoff");
    delay(5000);
    ESP.restart();
  }
  if (handoffValid) {
    Serial.println("[HANDOFF] Pending production installation found");
    display.showOtaStatus("Activating", "Confirming installation");
    if (!completeProvisionerHandoff(handoff)) {
      display.showOtaStatus("Activation Failed", "Will retry after restart");
      delay(5000);
      ESP.restart();
    }
    Serial.println("[HANDOFF] Installation confirmed; starting customer onboarding");
  }

  if (!wifi.connect(display)) {
    Serial.println("[WIFI] Restarting after provisioning failure");
    delay(3000);
    ESP.restart();
  }

  display.showConnected(Config::FIRMWARE_VERSION, WiFi.localIP(), hardwareId);
  Serial.println("[APP] Main screen loaded");

  DeviceConfiguration configuration;
  if (!DeviceConfiguration::load(configuration)) {
    Serial.println("[OTA] Disabled: no permanent device identity is stored");
    return;
  }
  ota.configure(configuration.platformUrl, configuration.deviceToken);

  if (otaNeedsTrustedClock(configuration.platformUrl) && !synchronizeClock()) {
    Serial.println("[OTA] Skipped because the TLS certificate cannot be validated");
    return;
  }

  attemptPendingInstallationConfirmation();
}

void loop() {
  if (pendingConfirmationRetry && WiFi.status() == WL_CONNECTED &&
      static_cast<long>(millis() - nextPendingConfirmationAttempt) >= 0) {
    attemptPendingInstallationConfirmation();
  }
  // A consuming firmware application owns its runtime behavior. The normal OTA
  // check runs once, after any pending installation has been confirmed.
  delay(100);
}
