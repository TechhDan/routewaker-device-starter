#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

class DisplayManager {
 public:
  void begin();
  void showSplash();
  void showProvisioning(const String& accessPointName, const IPAddress& portalIp);
  void showConnected(const char* firmwareVersion, const IPAddress& deviceIp);

 private:
  void drawQrCode(const String& value, int16_t centerX, int16_t centerY);
  String majorVersionLabel(const char* semanticVersion) const;

  TFT_eSPI display_;
};
