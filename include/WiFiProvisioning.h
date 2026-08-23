#pragma once

#include <Arduino.h>
#include <WiFi.h>

class DisplayManager;

class WiFiProvisioning {
 public:
  bool connect(DisplayManager& display);

 private:
  bool hasSavedCredentials() const;
};
