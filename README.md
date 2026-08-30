# RouteWaker OTA Device Starter

This repository is a reference ESP32 firmware application for integrating a
device with the RouteWaker OTA lifecycle. It is intentionally a small,
working blueprint that application firmware can copy or adapt; it is not the
tool used to register devices in the OTA platform.

## Responsibility boundary

The **RouteWaker Device Provisioner** owns device registration and credential
issuance. Use it before installing an application derived from this starter.
The resulting per-device bearer token is injected into the firmware build via
the ignored `include/ota_secrets.h` file.

This starter owns the device-side OTA lifecycle:

1. Connect to Wi-Fi, using a local setup access point when network credentials
   have not been saved.
2. Authenticate to the OTA API with a previously issued device token.
3. Report the running semantic firmware version by heartbeat.
4. Ask the platform whether a compatible release is available.
5. Download the artifact to the inactive OTA partition.
6. Verify its declared size and SHA-256 digest before activation.
7. Restart into the update and report installation success on the next boot.
8. Report download and installation telemetry to the platform.

Wi-Fi setup in this project is network onboarding only. It does not create,
register, claim, or provision a device record in the RouteWaker OTA platform.

## Using this as an application blueprint

The reusable OTA implementation is in `OtaManager`. `main.cpp` demonstrates
the expected boot order and `DisplayManager` demonstrates optional user
feedback. A real firmware application should retain the OTA calls while
replacing the starter screen and empty runtime loop with its product-specific
behavior.

Copy `include/ota_secrets.example.h` to `include/ota_secrets.h` and provide the
token issued for the device:

```cpp
#define ROUTEWAKER_OTA_DEVICE_TOKEN "rwd_token_issued_by_the_provisioner"
```

Production builds should also provide `ROUTEWAKER_OTA_ROOT_CA`. See
`OTA_TESTING.md` for the end-to-end release test procedure.

## What does not belong here

- Device registration or claiming workflows
- Registration URLs or registration QR codes
- Operator/admin authentication
- Manufacturing inventory management
- Product-specific firmware features

Those concerns should remain in the Device Provisioner or the consuming
firmware application.
