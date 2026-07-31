# Raspberry Pi Command Center Planning

Date: 2026-07-25

## Goal

Start Priority #6 by choosing the first Raspberry Pi integration path before adding firmware code.

The first milestone should prove that the Cardputer can safely talk to the Raspberry Pi home IoT stack without replacing the existing local utility features. Command publishing should be added only as explicit whitelisted JSON actions.

## Current Context

- Cardputer WiFi Connect works from microSD `/config/wifi.txt`.
- The nearby Raspberry Pi learning repo already has an MQTT broker/listener architecture.
- The Pi listener subscribes to `home/#`.
- The existing ESP32-C3 test node publishes to `home/devices/esp32-c3-test/...`.
- The Pi dashboard and health monitor are file/dashboard oriented, not a direct device API.

## Transport Decision

Use Wi-Fi MQTT for the first Raspberry Pi command center milestone.

Why MQTT first:

- The Raspberry Pi project already uses MQTT as its device message bus.
- MQTT matches the existing `home/devices/<device>/...` topic layout.
- The Cardputer can subscribe to device status without requiring a new Pi HTTP API.
- Later command actions can publish to the already-existing `commands` topics.
- It keeps the first milestone local-network-only and beginner-friendly.

Rejected for the first milestone:

- HTTP: useful later for dashboard/API access, but the Pi dashboard currently serves HTML rather than a small JSON API.
- WebSocket: unnecessary until there is a richer live UI.
- USB serial: useful for direct bench control, but not a wireless command center.
- BLE: possible later, but not aligned with the existing Pi MQTT stack.

## First Firmware Milestone

Add a read-only `Pi Monitor` screen.

Implementation note: the first firmware pass was added on 2026-07-27. User confirmed Pi Monitor MQTT viewing works on Cardputer hardware on 2026-07-29. The first whitelisted `read_now` command publisher was added and confirmed working on Cardputer hardware on 2026-07-30.

The first firmware version should:

- Reuse the existing WiFi Connect foundation.
- Require Wi-Fi to already be connected.
- Read Pi/MQTT settings from microSD `/config/pi.txt`.
- Connect to the Raspberry Pi MQTT broker.
- Subscribe to `home/#` for read-only visibility while testing.
- Treat `home/devices/<device>/<kind>` topics as structured device-list updates.
- Show broker status, message count, last topic/payload, and a compact device list on the Cardputer display.
- Keep Backspace returning safely to the main menu.
- Do not implement direct shell control, remote command execution, or Pi admin actions.

Suggested `/config/pi.txt` format:

```text
mqtt_host=10.0.0.180
mqtt_port=1883
device_id=scoober-cardputer
command_target=esp32-c3-test
```

`mqtt_host` should be the Raspberry Pi broker IP or resolvable hostname. `command_target` should be the explicit device id used in `home/devices/<command_target>/commands`.

## First Command Action

Add one whitelisted MQTT command publisher before adding any target picker or free-form controls.

- Press `C` in Pi Monitor to publish `{"command":"read_now"}`.
- Publish only to `home/devices/<command_target>/commands`.
- Keep `home/#` subscribed so message count and last topic/payload still show incoming responses or telemetry.
- Show response topics such as `home/devices/<device>/responses` on a dedicated `Resp:` line.
- Do not publish arbitrary command text.
- Do not implement direct shell control, remote command execution, or Pi admin actions.
- Confirmed working on Cardputer hardware on 2026-07-30.

## First MQTT Identity

Cardputer identity:

```text
scoober-cardputer
```

Optional status publish topic after the monitor and first whitelisted command are stable:

```text
home/devices/scoober-cardputer/status
```

Suggested status payload:

```json
{"device":"scoober-cardputer","firmware_version":"v0.1.0","uptime_ms":123456,"wifi_rssi":-57,"free_heap":180000}
```

## Later Milestones

After the read-only monitor and first command work on hardware:

1. Add target selection for known devices.
2. Hardware-test command response display from `home/devices/<device>/responses` if the generic `Last`/`Pay` display is not enough.
3. Publish Cardputer status/availability to MQTT.
4. Add another safe command action for the ESP32-C3 test node, such as `set_interval`.
5. Consider a Pi-side JSON API only if the Cardputer needs dashboard data that MQTT does not already provide.
6. Revisit MQTT authentication after Mosquitto configuration is confirmed on the Raspberry Pi.

## Acceptance Checks

Before calling the first firmware milestone complete:

- Missing `/config/pi.txt` shows a helpful message and does not crash.
- Wi-Fi disconnected shows a helpful message pointing back to WiFi Connect.
- Bad broker IP/port fails gracefully without locking the UI.
- Correct broker settings show MQTT connected.
- Incoming `home/#` messages increment `Msgs` and update `Last`/`Pay`.
- Incoming `home/devices/<device>/<kind>` messages update the device list.
- Incoming `home/devices/<device>/responses` messages update `Resp:`.
- With `command_target=esp32-c3-test`, pressing `C` publishes `read_now` to `home/devices/<command_target>/commands`.
- Backspace returns to the menu.
- No Wi-Fi or MQTT passwords are hardcoded in source.
- Real local `/config/pi.txt` files are ignored by Git.
