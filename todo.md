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

- Keep saved Wi-Fi as SSID-only unless the user explicitly asks to change it.
- Approved credential strategy: read Wi-Fi credentials from microSD `/config/wifi.txt`.
- Planned config format:
  - `ssid=YourNetworkName`
  - `password=YourNetworkPassword`
- Do not hardcode credentials in source code.
- Do not store Wi-Fi passwords in Preferences/NVS.
- Do not commit real `wifi.txt` files; `.gitignore` excludes `/config/wifi.txt` and `/wifi.txt` for accidental local copies.
- Do not add `WiFi.begin` until implementing an intentional connect screen/helper.
- Future connect behavior should show clear status, timeout gracefully, handle missing SD/config gracefully, and return safely to the menu.

## Priority 6: Raspberry Pi Command Center Planning

- Define the first Pi integration goal before coding.
- Decide transport:
  - Wi-Fi HTTP
  - Wi-Fi MQTT
  - WebSocket
  - USB serial
  - BLE
- Decide whether the Cardputer is primarily a dashboard, command launcher, logger, or remote shell front-end.
- Do not start this until current local utility features are stable.

## Priority 7: External Display Revisit

- External display support is intentionally inactive.
- Do not use G8/G9 directly for external I2C.
- If revisiting SSD1309 OLED:
  - choose safe pins or use Grove with an I2C mux/expander
  - add U8g2 back only when the hardware plan is approved
  - add a simple proof-of-life screen first
  - verify keyboard navigation after wiring

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
