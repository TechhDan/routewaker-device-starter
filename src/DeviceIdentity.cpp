#include "DeviceIdentity.h"

#include "config.h"

namespace DeviceIdentity {

String hardwareId() {
  const uint64_t efuseMac = ESP.getEfuseMac();
  char id[16];
  snprintf(id, sizeof(id), "RW-%04X%08X",
           static_cast<uint16_t>(efuseMac >> 32),
           static_cast<uint32_t>(efuseMac));
  return String(id);
}

String setupAccessPointName() {
  const String id = hardwareId();
  return String(Config::SETUP_AP_NAME) + "-" + id.substring(id.length() - 6);
}

String provisioningUrl() {
  return String(Config::DEVICE_PROVISIONING_URL) +
         "?hardware_id=" + hardwareId() +
         "&firmware_version=" + Config::FIRMWARE_VERSION;
}

}  // namespace DeviceIdentity
