#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

class DisplayManager {
 public:
  void begin();
  void showSplash(const String& hardwareId);
  void showProvisioning(const String& accessPointName, const IPAddress& portalIp);
  void showConnected(const char* firmwareVersion, const IPAddress& deviceIp,
                     const String& hardwareId);
  void showOtaStatus(const String& title, const String& detail = "");
  void showOtaProgress(size_t received, size_t total);

 private:
  void drawQrCode(const String& value, int16_t centerX, int16_t centerY);
  String majorVersionLabel(const char* semanticVersion) const;

  TFT_eSPI display_;
};
