#pragma once

#include <Arduino.h>

struct DeviceConfiguration {
  String platformUrl;
  String deviceToken;

  bool isValid() const;
  static bool load(DeviceConfiguration& configuration);
  static bool save(const String& platformUrl, const String& deviceToken);
};
