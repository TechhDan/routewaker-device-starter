# Route Waker OTA test setup

The firmware implements the device API contract from
`TechhDan/routewaker-device-platform`:

1. Authenticate with the per-device bearer token.
2. Send the current semantic version to `POST /api/device/heartbeat`.
3. Check `GET /api/device/firmware-update`.
4. Download the authenticated artifact into the inactive OTA partition.
5. Verify its declared byte length and SHA-256 before activation.
6. Report download and installation telemetry.

## Register a test device

Register the device with the **RouteWaker Device Provisioner**. Registration is
deliberately external to this starter firmware; this repository only consumes
the per-device credential produced by that workflow.

Use a unique hardware ID. The device model, hardware revision, and release
channel must exactly match the firmware release you later create. A suitable
first test identity is:

```text
Model: route-waker-display
Hardware revision: esp32-2432s028-v1
Release channel: stable
```

Install the starter through the Device Provisioner. It writes the temporary
installer network plus the platform URL, device token, release ID, and expected
version to `rw-prov`. The starter permanently copies only the URL and token.

The runtime platform URL always comes from the provisioner handoff and is
stored permanently in the `rw-device` NVS namespace. For local development,
configure the provisioner with the locally reachable platform URL before it
installs the starter. Use the development computer's LAN address rather than
`localhost`, because `localhost` on the device refers to the ESP32. Update the
provisioner configuration if the development computer's DHCP lease changes.

The platform's `APP_URL` must match that provisioner-supplied base URL so the
returned artifact URL is reachable by the ESP32. `ota_secrets.h` is ignored by
Git and is used only for TLS root CA configuration.

For production, also configure `ROUTEWAKER_OTA_ROOT_CA`. Without a CA, HTTPS
is encrypted but the server certificate is not authenticated; the firmware
logs a warning on every request. When a CA is configured for an HTTPS API, the
firmware synchronizes its UTC clock with `pool.ntp.org` (falling back to
`time.nist.gov`) before making OTA requests. If time synchronization fails,
secure OTA is skipped rather than attempting certificate validation with an
invalid clock.

Define the PEM certificate as adjacent quoted C strings. Include `\n` after
every PEM line and a trailing `\` on every macro line except the last; see
`include/ota_secrets.example.h`. A multiline raw string directly inside a
`#define` is not valid without preprocessor line continuations.

## Run the v1 to v2 test

1. Publish firmware `1.0.0` and install it through the Device Provisioner.
2. Confirm the serial log shows handoff confirmation followed by
   `[OTA] Heartbeat accepted`, then complete customer Wi-Fi onboarding.
3. Change `FIRMWARE_VERSION` in `include/config.h` to `2.0.0` and build again.
4. Calculate the SHA-256 of `.pio/build/esp32dev/firmware.bin`.
5. In the platform, create a `2.0.0` release whose model, hardware revision,
   and channel match the device; upload that exact binary and SHA-256.
6. Publish the release and set a rollout percentage that includes the device
   (100 percent is simplest for the first test).
7. Restore the local source version to `1.0.0` if necessary, and reboot the
   USB-installed v1 device.

The device checks once after each boot. When offered v2, it displays download
progress, verifies and activates the inactive slot, restarts, displays `v2`,
heartbeats `2.0.0`, and reports `installation_succeeded`.
