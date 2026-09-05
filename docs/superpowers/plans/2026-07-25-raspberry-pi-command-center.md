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

Implementation note: the first firmware pass was added on 2026-07-27. User confirmed Pi Monitor MQTT viewing works on Cardputer hardware on 2026-07-29. The first whitelisted `read_now` command publisher was added and confirmed working on Cardputer hardware on 2026-07-30. Cardputer status/availability publishing was added on 2026-08-08 and confirmed working on hardware on 2026-08-15. Fixed-choice `set_interval` publishing was added and confirmed working on hardware on 2026-08-15. Target selection was added and confirmed working on hardware on 2026-08-15. The Pi Monitor UI was expanded to a project list, command list, and Home / Diagnostics split so new MQTT projects can be added cleanly. The Pi Monitor OLED was then changed to a message-only 5x7 font view with one header and manual `S` paging through wrapped MQTT topic/payload text.

The first firmware version should:

- Reuse the existing WiFi Connect foundation.
- Require Wi-Fi to already be connected.
- Read Pi/MQTT settings from microSD `/config/pi.txt`.
- Connect to the Raspberry Pi MQTT broker.
- Subscribe to `home/#` for read-only visibility while testing.
- Treat `home/devices/<device>/<kind>` topics as structured device-list updates.
- Show broker status, message count, last topic/payload, and compact project/device status on the Cardputer display.
- Open on a project list with `Home / Diagnostics`, configured projects, and discovered device IDs.
- Open a command list for the selected project.
- Use the OLED as a message-only viewer with one header plus manually paged MQTT text: selected-project messages in project command view and all MQTT messages in `Home / Diagnostics`.
- Keep Backspace returning safely to the main menu.
- Do not implement direct shell control, remote command execution, or Pi admin actions.

Suggested `/config/pi.txt` format:

```text
mqtt_host=10.0.0.180
mqtt_port=1883
device_id=scoober-cardputer
command_target=esp32-c3-test
project=esp32-c3-test|ESP32-C3 Test|basic
```

`mqtt_host` should be the Raspberry Pi broker IP or resolvable hostname. `command_target` should be the explicit device id used in `home/devices/<command_target>/commands`. `command_target` remains compatible as the initial/default target, while `project=id|Label|profile` is the scalable project entry format. The initial firmware command profile is `basic`, with `read_now` and fixed-choice `set_interval`.

## First Command Action

Add whitelisted MQTT command publishers before adding any target picker or free-form controls.

- Press `C` in Pi Monitor to publish `{"command":"read_now"}`.
- Press `T` to cycle the command target through `command_target` and discovered device IDs.
- Press `I` to cycle fixed `set_interval` choices: 10, 30, 60, and 300 seconds.
- Press OK on the selected `set_interval` command to publish `{"command":"set_interval","seconds":<selected>}`.
- Press `S` to advance the OLED MQTT message page manually.
- Press arrow keys to scroll the current project list or command list.
- Press OK to open a highlighted project or send the highlighted command; in `Home / Diagnostics`, OK retries MQTT.
- Press Backspace to return from a Pi Monitor subview to the project list before leaving Pi Monitor.
- Publish only to `home/devices/<command_target>/commands`.
- Keep `home/#` subscribed so message count and last topic/payload still show incoming responses or telemetry.
- Show response topics such as `home/devices/<device>/responses`, nested response topics, or post-command updates from `command_target` on a dedicated `Resp:` line.
- User reported on 2026-08-06 that `Resp:` still stayed on `waiting`; do not block the milestone on this while `Last` / `Pay` show command feedback.
- Do not publish arbitrary command text.
- Do not accept free-form interval input for `set_interval`.
- Do not implement direct shell control, remote command execution, or Pi admin actions.
- `read_now` was confirmed working on Cardputer hardware on 2026-07-30.
- `set_interval` was confirmed working on Cardputer hardware on 2026-08-15.
- Target selection was confirmed working on Cardputer hardware on 2026-08-15.

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

Availability topic:

```text
home/devices/scoober-cardputer/availability
```

Availability payloads are retained `online` / `offline`, with an MQTT last-will set to `offline`.
User confirmed status/availability publishing works on Cardputer hardware on 2026-08-15.

## Later Milestones

After the read-only monitor and first command work on hardware:

1. Treat Priority #6 as complete enough unless the Pi-side listener needs new command support.
2. Revisit command response display only if the generic `Last`/`Pay` display is not enough for a future command.
3. Revisit Pi-side command support only if a target does not handle the selected command.
4. Consider a Pi-side JSON API only if the Cardputer needs dashboard data that MQTT does not already provide.
5. Revisit MQTT authentication after Mosquitto configuration is confirmed on the Raspberry Pi.
6. Add new command profiles only as explicit firmware definitions, then map projects to them with `project=id|Label|profile`.

## Acceptance Checks

Before calling the first firmware milestone complete:

- Missing `/config/pi.txt` shows a helpful message and does not crash.
- Wi-Fi disconnected shows a helpful message pointing back to WiFi Connect.
- Bad broker IP/port fails gracefully without locking the UI.
- Correct broker settings show MQTT connected.
- Incoming `home/#` messages increment `Msgs` and update `Last`/`Pay`.
- Incoming `home/devices/<device>/<kind>` messages update the device list.
- The project list shows `Home / Diagnostics`, configured projects, and discovered device IDs.
- The command list opens for the selected project.
- `Home / Diagnostics` shows diagnostics on the LCD and one-header manually paged all-message MQTT text on the OLED.
- A selected project shows command controls on the LCD and one-header manually paged project-filtered MQTT text on the OLED.
- Incoming `home/devices/<device>/responses`, nested response topics, or post-command `command_target` updates update `Resp:`.
- Cardputer publishes retained status to `home/devices/scoober-cardputer/status`.
- Cardputer publishes retained availability to `home/devices/scoober-cardputer/availability`.
- With `command_target=esp32-c3-test`, pressing `C` publishes `read_now` to `home/devices/<command_target>/commands`.
- Pressing `T` cycles through `command_target` and discovered device IDs, excluding the Cardputer's own MQTT identity.
- User confirmed target selection works on Cardputer hardware on 2026-08-15.
- Pressing `I` cycles fixed `set_interval` choices, pressing OK publishes the selected command to `home/devices/<command_target>/commands`, and pressing `S` advances the OLED MQTT message page.
- Backspace returns to the menu.
- No Wi-Fi or MQTT passwords are hardcoded in source.
- Real local `/config/pi.txt` files are ignored by Git.
