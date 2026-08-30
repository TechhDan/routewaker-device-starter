#pragma once

#include <Arduino.h>

namespace DeviceIdentity {

// Stable for the lifetime of the ESP32 because it is derived from the
// factory-programmed eFuse MAC address.
String hardwareId();
String setupAccessPointName();
String provisioningUrl();

}  // namespace DeviceIdentity
