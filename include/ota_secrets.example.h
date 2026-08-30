#pragma once

// Copy this file to include/ota_secrets.h. That file is ignored by Git.
// Production defaults to https://ota.routewaker.com in config.h. Override the
// URL here only when testing against a locally hosted platform reachable by
// the device (localhost will refer to the ESP32 itself).
// #define ROUTEWAKER_OTA_API_BASE_URL "http://192.168.1.98:8000"
// The platform URL override is only a development default. Production
// devices receive their runtime URL and token from the provisioner handoff.

// Recommended for production: provide the PEM root CA that signs the API URL.
// Leave undefined during initial testing to use insecure TLS (logged at boot).
// Each quoted certificate line must end in \n and each macro line except the last
// must end in a backslash, as shown below.
// #define ROUTEWAKER_OTA_ROOT_CA \
//     "-----BEGIN CERTIFICATE-----\n" \
//     "paste each Base64 line here\n" \
//     "-----END CERTIFICATE-----\n"
