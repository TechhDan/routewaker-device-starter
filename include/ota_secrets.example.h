#pragma once

// Copy this file to include/ota_secrets.h. That file is ignored by Git.
// Production defaults to https://ota.routewaker.com in config.h. Override the
// URL here only when testing against a locally hosted platform reachable by
// the device (localhost will refer to the ESP32 itself).
// #define ROUTEWAKER_OTA_API_BASE_URL "http://192.168.1.98:8000"
#define ROUTEWAKER_OTA_DEVICE_TOKEN "rwd_replace_with_device_create_token"

// Recommended for production: provide the PEM root CA that signs the API URL.
// Leave undefined during initial testing to use insecure TLS (logged at boot).
// #define ROUTEWAKER_OTA_ROOT_CA R"PEM(
// -----BEGIN CERTIFICATE-----
// ...
// -----END CERTIFICATE-----
// )PEM"
