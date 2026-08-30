#include "WiFiProvisioning.h"

#include <WiFiManager.h>

#include "DeviceIdentity.h"
#include "DisplayManager.h"
#include "config.h"

namespace {
DisplayManager* provisioningDisplay = nullptr;

void onPortalStarted(WiFiManager* manager) {
  Serial.println("[WIFI] Starting provisioning mode");
  Serial.printf("[WIFI] Access point: %s\n", manager->getConfigPortalSSID().c_str());
  Serial.printf("[WIFI] Setup page: http://%s\n", WiFi.softAPIP().toString().c_str());

  if (provisioningDisplay != nullptr) {
    provisioningDisplay->showProvisioning(manager->getConfigPortalSSID(), WiFi.softAPIP());
  }
}
}  // namespace

bool WiFiProvisioning::connect(DisplayManager& display) {
  Serial.println("[WIFI] Checking saved credentials");
  Serial.printf("[WIFI] Saved credentials: %s\n", hasSavedCredentials() ? "yes" : "no");

  WiFi.mode(WIFI_STA);

  WiFiManager manager;
  manager.setDebugOutput(false);
  manager.setConnectTimeout(Config::WIFI_CONNECT_TIMEOUT_SECONDS);
  manager.setConfigPortalTimeout(Config::PORTAL_TIMEOUT_SECONDS);
  manager.setAPCallback(onPortalStarted);

  provisioningDisplay = &display;
  const String accessPointName = DeviceIdentity::setupAccessPointName();
  const bool connected = manager.autoConnect(accessPointName.c_str());
  provisioningDisplay = nullptr;

  if (!connected) {
    Serial.println("[WIFI] Connection or provisioning failed");
    return false;
  }

  Serial.println("[WIFI] Connected");
  Serial.printf("[WIFI] SSID: %s\n", WiFi.SSID().c_str());
  Serial.printf("[WIFI] IP: %s\n", WiFi.localIP().toString().c_str());
  return true;
}

bool WiFiProvisioning::hasSavedCredentials() const {
  return WiFi.SSID().length() > 0;
}
