# Scoober Todo

Prioritized next tasks for the project. Keep this file current so a new Codex session can continue without relying on chat history.

## Priority 1: Stabilize Current Hardware Features

- Current hardware features are stable enough to move forward.
- Battery readings can still be quirky with ENV III attached, but `Charge:` is good enough for now.
- Revisit menu and features to clean them up.

## Priority 2: Environment Feature Improvements

- Consider units/settings: C/F toggle, pressure hPa/inHg toggle, refresh interval.

## Priority 3: NRF24L01 Module Integration

- Current RF Scan milestone is complete enough to move on; user confirmed it works on hardware on 2026-07-15.
- Do not resume the XIAO two-node send/receive proof unless the user explicitly asks.
- The XIAO ESP32-C3 OLED send/receive proof is retired for now after asymmetric behavior could not be resolved locally.
- Current firmware milestone uses a simple RF channel scanner:
  - Sweep channels 0-125
  - show a per-channel activity bar graph
  - highlight quiet channels for future NRF24 projects

## Priority 4: Project Structure Cleanup

- Initial project structure cleanup is done.
- `src/main.cpp` now contains only `setup()` and `loop()`.
- Shared declarations live in `src/app.h`; shared globals/menu state live in `src/app_state.cpp`.
- Feature code is split into `src/ui.cpp`, `src/input.cpp`, `src/power_screen.cpp`, `src/wifi_screens.cpp`, `src/voice_memos.cpp`, `src/environment_screen.cpp`, `src/rf_scanner.cpp`, and `src/level_tool.cpp`.
- Firmware guard scripts scan all source files through `tools/firmware_source.py`.
- Future cleanup should stay incremental and only happen after guards/build pass.

## Priority 5: Saved Wi-Fi And Networking Prep

- Current Priority #5 milestone is complete enough to move on; user confirmed WiFi Connect testing worked great on Cardputer hardware on 2026-07-23.
- Keep saved Wi-Fi as SSID-only unless the user explicitly asks to change it.
- Credential strategy: read Wi-Fi credentials from microSD `/config/wifi.txt`.
- Config format:
  - `ssid=YourNetworkName`
  - `password=YourNetworkPassword`
- Do not hardcode credentials in source code.
- Do not store Wi-Fi passwords in Preferences/NVS.
- Do not commit real `wifi.txt` files; `.gitignore` excludes `/config/wifi.txt` and `/wifi.txt` for accidental local copies.
- `WiFi.begin` is allowed only in the intentional WiFi Connect flow.
- Future networking work should build on this WiFi Connect flow instead of adding a second credential path.

## Priority 6: Raspberry Pi Command Center Planning

- Priority #6 planning started on 2026-07-25.
- First firmware pass added on 2026-07-27: read-only `Pi Monitor` screen before sending commands.
- User confirmed Pi Monitor MQTT viewing works on Cardputer hardware on 2026-07-29.
- First whitelisted `read_now` command publisher added on 2026-07-30.
- User confirmed Pi Monitor `read_now` command publishing works on Cardputer hardware on 2026-07-30.
- User confirmed Cardputer MQTT status/availability publishing works on hardware on 2026-08-15.
- Fixed-choice `set_interval` command publisher added on 2026-08-15.
- User confirmed Pi Monitor `set_interval` command publishing works on hardware on 2026-08-15.
- Target selection added on 2026-08-15.
- User confirmed Pi Monitor target selection works on hardware on 2026-08-15.
- Transport decision: Wi-Fi MQTT, because the Raspberry Pi learning repo already uses an MQTT broker/listener and `home/devices/<device>/...` topics.
- First config file: microSD `/config/pi.txt`.
- Config format:
  - `mqtt_host=10.0.0.180`
  - `mqtt_port=1883`
  - `device_id=scoober-cardputer`
  - `command_target=esp32-c3-test`
- First subscription:
  - `home/#`
- First screen behavior:
  - require Wi-Fi to already be connected through WiFi Connect
  - show MQTT broker connection status
  - show message count, last topic/payload, and command response display for incoming `home/#` messages
  - show a compact device list/status view
  - update the device list for `home/devices/<device>/<kind>` topics
  - show response topics such as `home/devices/<device>/responses`, nested response topics, and post-command `command_target` updates on a dedicated `Resp:` line
  - publish Cardputer availability to `home/devices/scoober-cardputer/availability`
  - publish Cardputer status JSON to `home/devices/scoober-cardputer/status`
  - publish whitelisted `read_now` to `home/devices/<command_target>/commands` with `C`
  - cycle command targets with `T`
  - cycle fixed `set_interval` choices with `I`: 10, 30, 60, and 300 seconds
  - publish whitelisted `set_interval` to `home/devices/<command_target>/commands` with `S`
  - fail gracefully when Wi-Fi, SD config, or broker connection is missing
  - keep Backspace return-to-menu behavior
- Controls:
  - OK retries MQTT connection
  - `C` publishes `{"command":"read_now"}` to the configured command target
  - `T` cycles command target between configured and discovered devices
  - `I` cycles fixed `set_interval` choices
  - `S` publishes `{"command":"set_interval","seconds":<selected>}` to the configured command target
  - `R` clears the device list and reconnects
  - `D` disconnects MQTT
- Hardware test checklist:
  - MQTT connect and incoming message display confirmed on hardware on 2026-07-29
  - `read_now` command publishing confirmed on hardware on 2026-07-30
  - Cardputer status/availability publishing confirmed on hardware on 2026-08-15
  - `set_interval` command publishing confirmed on hardware on 2026-08-15
  - target selection confirmed on hardware on 2026-08-15
  - `Resp:` still stayed on `waiting` during 2026-08-06 hardware testing; not blocking while `Last` / `Pay` show command feedback
  - missing `/config/pi.txt`, Wi-Fi disconnected, and wrong broker IP/port still worth spot-checking after future edits
- Do not implement direct shell control, remote command execution, Pi admin actions, arbitrary command entry, or risky commands.
- Priority #6 is complete enough unless the Pi-side listener needs new command support.
- Revisit `Resp:` only if future commands need explicit success/failure acknowledgments beyond `Last` / `Pay`.

## Priority 7: External Display Revisit

- Priority #7 planning started on 2026-08-15.
- External display support is intentionally inactive.
- Do not use G8/G9 directly for external I2C.
- Planning doc: `docs/superpowers/plans/2026-08-15-external-display-revisit.md`.
- Guard: `tools/check_external_display_revisit.py`.
- User chose the M5Stack Unit PaHub v2.1 on 2026-08-15.
- Chosen hardware path:
  - Cardputer Grove G2/G1 to Unit PaHub v2.1 input
  - default PaHub address `0x70`
  - ENV III on PaHub channel 0
  - OLED reserved for PaHub channel 1
- First firmware foundation added:
  - detect PaHub at `0x70`
  - select ENV III channel 0 before Environment init/read
  - fall back to direct Grove when no PaHub is present
- If revisiting SSD1309 OLED:
  - choose safe pins or use Grove with an I2C mux/expander
  - add U8g2 back only when the hardware plan is approved
  - add a simple proof-of-life screen first
  - verify keyboard navigation after wiring
- Hardware test checklist:
  - ENV III through Unit PaHub v2.1 channel 0: needs testing
  - keyboard navigation with Unit PaHub v2.1 attached: needs testing
  - direct Grove ENV III fallback after this change: worth spot-checking

## Done / Historical Milestones

- Created starter PlatformIO firmware for Cardputer Adv.
- Renamed banner/menu identity to Scoober.
- Removed Keyboard Test, Display Test, SD Card Test, and ESP-NOW RC Placeholder.
- Split Battery and System into separate screens.
- Added arrow/OK menu navigation.
- Fixed display flicker using content canvas redraws.
- Tuned Battery charging estimate with voltage trend threshold and continuous sampling.
- Added a local Battery trend regression guard and clear-on-drop logic for charge stop behavior.
- Raised Battery trend threshold to ignore observed unplugged +26 mV drift and stopped using percentage jumps as charging evidence.
- Tried and reverted a dedicated ENV III I2C bus after QMP6988 pressure/altitude failed on hardware.
- Made Battery `Charge:` ignore raw API charging when the filtered voltage trend does not confirm it.
- Added a VBUS-present gate before Battery can show `Charging`.
- Added scrollable WiFi Scan.
- Added persistent Saved WiFi names with delete flow.
- Added Voice Memos to microSD.
- Added Level tool using IMU crosshair/dot display.
- Tried and removed active OLED support.
- Added Environment screen for M5Stack ENV III Unit.
- Added optional Environment CSV logging to microSD with one new `/env/envNNN.csv` file per session.
- Environment CSV columns: uptime seconds, temperature C, temperature F, humidity percent, pressure hPa, altitude m.
- Added Environment log naming before start, `L` start/stop logging control, log file/sample display, and graceful SD-missing behavior.
- Added first-pass NRF24 diagnostics feature with RF24 dependency, EXT shared SPI pin plan, graceful missing-radio status, manual test transmit, passive receive counter, and local guard script.
- Added XIAO ESP32-C3 NRF24/OLED second-node PlatformIO project with `CARD ping N` / `XIAO ack N` proof protocol.
- Tuned NRF24 proof firmware after hardware testing showed payloads could arrive even when RF24 hardware ACK was missed: switched to 250 kbps, disabled RF hardware ACK for proof mode, simplified to shared `SCBR1` address, added XIAO beacons, added Cardputer RPD/carrier counters, lowered XIAO PA for close-range testing, made the XIAO send repeated delayed replies, and added TX success/failure plus RX pipe/FIFO diagnostics.
- Retired the active XIAO two-node proof after testing showed Cardputer-to-XIAO worked, XIAO TX/OK increased with no failures, but Cardputer did not decode XIAO beacons or replies.
- Replaced the active NRF24 diagnostics screen with an RF channel scanner that sweeps 0-125, draws a bar graph, and highlights quiet channels.
- User confirmed RF Scan is working fine on Cardputer hardware on 2026-07-15.
- Split the Cardputer firmware out of monolithic `src/main.cpp` into shared app state plus feature modules while keeping guards and PlatformIO build passing.
- Main menu header now reads `Scoober (Use arrows, OK to select)` and menu rows were shifted up to fit cleanly.
- Voice Memos and Environment titles now carry their first-line context, freeing content space for feature data.
- Priority #5 credential strategy documented as microSD `/config/wifi.txt`, with guard coverage before connection firmware is added.
- Added WiFi Connect screen using microSD `/config/wifi.txt`, graceful missing-config behavior, timeout-based `WiFi.begin`, IP display, retry, and disconnect controls.
- User confirmed WiFi Connect testing worked great on Cardputer hardware on 2026-07-23.
- Planned Priority #6 as a Wi-Fi MQTT Raspberry Pi command center starting with a read-only Pi Monitor.
- Added first-pass Pi Monitor screen using microSD `/config/pi.txt`, read-only MQTT monitoring, compact device list, graceful missing-config/Wi-Fi/broker behavior, retry, clear/reconnect, and disconnect controls.
- User confirmed Pi Monitor MQTT viewing works on Cardputer hardware on 2026-07-29.
- Added first whitelisted Pi Monitor command publisher: `C` sends `read_now` to `home/devices/<command_target>/commands`.
- User confirmed Pi Monitor `read_now` command publishing works on Cardputer hardware on 2026-07-30.
- Broadened Pi Monitor `Resp:` display for response topics and post-command command target updates.
- User reported `Resp:` still stayed on `waiting` on 2026-08-06; leave it as non-blocking unless explicit acknowledgments become important.
- Added Cardputer MQTT status/availability publishing under `home/devices/scoober-cardputer/...`.
- User confirmed Cardputer MQTT status/availability publishing works on hardware on 2026-08-15.
- Added fixed-choice Pi Monitor `set_interval` command publisher using `I` to cycle and `S` to send.
- User confirmed Pi Monitor `set_interval` command publishing works on hardware on 2026-08-15.
- Added Pi Monitor target selection using `T` to cycle configured and discovered devices.
- User confirmed Pi Monitor target selection works on hardware on 2026-08-15.
