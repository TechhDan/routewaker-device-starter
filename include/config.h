#pragma once

#include <Arduino.h>

#if __has_include("ota_secrets.h")
#include "ota_secrets.h"
#endif

#ifndef ROUTEWAKER_OTA_API_BASE_URL
#define ROUTEWAKER_OTA_API_BASE_URL ""
#endif

#ifndef ROUTEWAKER_OTA_DEVICE_TOKEN
#define ROUTEWAKER_OTA_DEVICE_TOKEN ""
#endif

#ifndef ROUTEWAKER_OTA_ROOT_CA
#define ROUTEWAKER_OTA_ROOT_CA ""
#endif

namespace Config
{

    static constexpr char FIRMWARE_VERSION[] = "2.0.0";
    static constexpr char PRODUCT_NAME[] = "Route Waker";
    static constexpr char SETUP_AP_NAME[] = "Route Waker-Setup";

    static constexpr uint32_t SERIAL_BAUD_RATE = 115200;
    static constexpr uint32_t SPLASH_DURATION_MS = 2000;
    static constexpr uint32_t WIFI_CONNECT_TIMEOUT_SECONDS = 20;
    static constexpr uint32_t PORTAL_TIMEOUT_SECONDS = 0; // Keep setup open until configured.

    static constexpr char OTA_API_BASE_URL[] = ROUTEWAKER_OTA_API_BASE_URL;
    static constexpr char OTA_DEVICE_TOKEN[] = ROUTEWAKER_OTA_DEVICE_TOKEN;
    static constexpr char OTA_ROOT_CA[] = ROUTEWAKER_OTA_ROOT_CA;
    static constexpr uint16_t OTA_REQUEST_TIMEOUT_MS = 15000;
    static constexpr uint16_t OTA_DOWNLOAD_TIMEOUT_MS = 20000;

} // namespace Config
