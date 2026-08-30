# RouteWaker OTA Device Starter

This repository is a reference ESP32 firmware application for integrating a
device with the RouteWaker OTA lifecycle. It is intentionally a small,
working blueprint that application firmware can copy or adapt; it is not the
tool used to register devices in the OTA platform.

## Responsibility boundary

The **RouteWaker Device Provisioner** owns device registration and credential
issuance. Use it before installing an application derived from this starter.
The resulting platform URL and per-device bearer token are transferred through
the `rw-prov` NVS handoff when the provisioner installs this firmware.

This starter owns the device-side OTA lifecycle:

1. Complete any pending provisioner handoff, then connect to customer Wi-Fi,
   using a local setup access point when customer credentials have not been saved.
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

Devices obtain their API URL and permanent `rwd_...` token from the provisioner
handoff, and the starter stores them in the `rw-device` NVS namespace. This is
also the URL source during development: local testing requires a provisioner
configured with a platform URL reachable by the ESP32. `include/ota_secrets.h`
remains available for `ROUTEWAKER_OTA_ROOT_CA`; it never contains the platform
URL or device token. See
`OTA_TESTING.md` for the end-to-end release test procedure.

On the first production boot the starter connects only to the installer Wi-Fi,
confirms the installed release, heartbeats the running version, stores only the
platform URL and device token in `rw-device`, and then erases `rw-prov`,
`rw-bootstrap`, and SDK installer Wi-Fi credentials before customer onboarding.
Any activation failure retains the handoff for a later boot.

## What does not belong here

- Device registration or claiming workflows
- Registration URLs or registration QR codes
- Operator/admin authentication
- Manufacturing inventory management
- Product-specific firmware features

Those concerns should remain in the Device Provisioner or the consuming
firmware application.
