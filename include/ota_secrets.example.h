#pragma once

// Copy this file to include/ota_secrets.h. That file is ignored by Git.
// The platform URL and device token always come from the Device Provisioner
// handoff and are stored permanently in the rw-device NVS namespace. For local
// testing, configure the provisioner with a platform URL reachable by the ESP32
// (localhost would refer to the ESP32 itself).

// Recommended for production: provide the PEM root CA that signs the API URL.
// Leave undefined during initial testing to use insecure TLS (logged at boot).
// Each quoted certificate line must end in \n and each macro line except the last
// must end in a backslash, as shown below.
// #define ROUTEWAKER_OTA_ROOT_CA \
//     "-----BEGIN CERTIFICATE-----\n" \
//     "paste each Base64 line here\n" \
//     "-----END CERTIFICATE-----\n"
