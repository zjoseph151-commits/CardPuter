# Scoober Todo

Prioritized next tasks for the project. Keep this file current so a new Codex session can continue without relying on chat history.

## Priority 1: Stabilize Current Hardware Features

- Current hardware features are stable enough to move forward.
- Battery readings can still be quirky with ENV III attached, but `Charge:` is good enough for now.

## Priority 2: Environment Feature Improvements

- Consider units/settings: C/F toggle, pressure hPa/inHg toggle, refresh interval.

## Priority 3: NRF24L01 Module Integration

- Integrate the NRF24L01+ PA+LNA module with SMA antenna and breakout adapter.
- Treat this as both a wiring/power task and a firmware task.
- Confirm the wiring plan before coding:
  - VCC and GND
  - SPI SCK, MISO, MOSI
  - CSN/chip-select
  - CE
  - optional IRQ if needed later
- Treat the EXT SPI pins as shared with microSD:
  - shared SPI: G40 SCK, G39 MISO, G14 MOSI
  - microSD CS: G12
  - NRF24 CSN/chip-select: G5
- Avoid known active or risky pins:
  - do not reuse microSD CS G12 as NRF24 CSN
  - ENV III Grove I2C pins: G2 SDA, G1 SCL
  - internal/shared I2C pins G8/G9
- Decide whether NRF24L01 should use a separate SPI bus or share an SPI bus safely with a dedicated CSN pin.
- Current firmware milestone uses the Arduino `RF24` library and a simple RF channel scanner:
  - initialize radio
  - show wiring/config status
  - Sweep channels 0-125
  - show a per-channel activity bar graph
  - highlight quiet channels for future NRF24 projects
  - allow manual rescan
- Keep the first implementation graceful if the module is missing or wiring is wrong.
- Add local guard scripts for menu integration and NRF feature structure before hardware testing.
- The XIAO ESP32-C3 OLED send/receive proof is retired for now after asymmetric behavior could not be resolved locally.

## Priority 4: Project Structure Cleanup

- Split `src/main.cpp` into smaller feature files once current behavior is stable.
- Suggested future files:
  - `src/ui.h` / `src/ui.cpp`
  - `src/power_screen.h` / `src/power_screen.cpp`
  - `src/wifi_screens.h` / `src/wifi_screens.cpp`
  - `src/voice_memos.h` / `src/voice_memos.cpp`
  - `src/environment_screen.h` / `src/environment_screen.cpp`
  - `src/level_tool.h` / `src/level_tool.cpp`
- Only refactor after guard scripts are passing and the user is not actively testing a hardware issue.

## Priority 5: Saved Wi-Fi And Networking Prep

- Keep saved Wi-Fi as SSID-only until user approves credentials.
- Design a secure credential strategy before adding any `WiFi.begin` calls.
- Possible credential approaches:
  - on-device entry screen
  - SD config file excluded from git
  - temporary serial command setup
  - captive portal setup
- Add clear docs before any credential implementation.

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
