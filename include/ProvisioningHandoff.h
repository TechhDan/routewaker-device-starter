#pragma once

#include <Arduino.h>

struct ProvisioningHandoff {
  String ssid;
  String password;
  String platformUrl;
  String deviceToken;
  uint64_t releaseId = 0;
  String expectedVersion;
  String eventId;

  static bool isPending();
  static bool load(ProvisioningHandoff& handoff);
  static bool saveEventId(const String& eventId);
  static bool clear();
  static bool clearBootstrap();
};
