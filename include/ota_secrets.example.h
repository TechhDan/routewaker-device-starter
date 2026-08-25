#pragma once

// Copy this file to include/ota_secrets.h. That file is ignored by Git.
// APP_URL on the Laravel platform must match the publicly reachable HTTPS URL.
#define ROUTEWAKER_OTA_API_BASE_URL "https://devices.example.com"
#define ROUTEWAKER_OTA_DEVICE_TOKEN "rwd_replace_with_device_create_token"

// Recommended for production: provide the PEM root CA that signs the API URL.
// Leave undefined during initial testing to use insecure TLS (logged at boot).
// #define ROUTEWAKER_OTA_ROOT_CA R"PEM(
// -----BEGIN CERTIFICATE-----
// ...
// -----END CERTIFICATE-----
// )PEM"
