#pragma once

#include <Arduino.h>

namespace Config {

static constexpr char FIRMWARE_VERSION[] = "1.0.0";
static constexpr char PRODUCT_NAME[] = "Route Waker";
static constexpr char SETUP_AP_NAME[] = "Route Waker-Setup";

static constexpr uint32_t SERIAL_BAUD_RATE = 115200;
static constexpr uint32_t SPLASH_DURATION_MS = 2000;
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_SECONDS = 20;
static constexpr uint32_t PORTAL_TIMEOUT_SECONDS = 0;  // Keep setup open until configured.

}  // namespace Config
