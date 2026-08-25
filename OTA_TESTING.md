# Route Waker OTA test setup

The firmware implements the device API contract from
`TechhDan/routewaker-device-platform`:

1. Authenticate with the per-device bearer token.
2. Send the current semantic version to `POST /api/device/heartbeat`.
3. Check `GET /api/device/firmware-update`.
4. Download the authenticated artifact into the inactive OTA partition.
5. Verify its declared byte length and SHA-256 before activation.
6. Report download and installation telemetry.

## Configure a test device

On the Laravel platform, run:

```sh
php artisan device:create
```

Use a unique hardware ID. The device model, hardware revision, and release
channel must exactly match the firmware release you later create. A suitable
first test identity is:

```text
Model: route-waker-display
Hardware revision: esp32-2432s028-v1
Release channel: stable
```

Copy `include/ota_secrets.example.h` to `include/ota_secrets.h`, then set:

```cpp
#define ROUTEWAKER_OTA_API_BASE_URL "https://your-platform-host"
#define ROUTEWAKER_OTA_DEVICE_TOKEN "rwd_the_one_time_token"
```

`ota_secrets.h` is ignored by Git. The platform's `APP_URL` must be the same
public base URL so the returned artifact URL is reachable by the ESP32.

For production, also configure `ROUTEWAKER_OTA_ROOT_CA`. Without a CA, HTTPS
is encrypted but the server certificate is not authenticated; the firmware
logs a warning on every request.

## Run the v1 to v2 test

1. Configure the API URL and token and upload firmware `1.0.0` over USB.
2. Confirm the serial log contains `[OTA] Heartbeat accepted`.
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
