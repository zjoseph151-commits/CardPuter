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
- Avoid known active or risky pins:
  - SD card pins: G40 SCK, G39 MISO, G14 MOSI, G12 CS
  - ENV III Grove I2C pins: G2 SDA, G1 SCL
  - internal/shared I2C pins G8/G9
- Decide whether NRF24L01 should use a separate SPI bus or share an SPI bus safely with a dedicated CSN pin.
- Prefer the Arduino `RF24` library if it builds cleanly with ESP32-S3 / PlatformIO.
- First firmware milestone should be a simple `NRF24` diagnostics feature:
  - initialize radio
  - show wiring/config status
  - show channel, data rate, PA level, and address
  - send a test packet
  - listen for a test packet
  - show packet counters and last packet text
- Keep the first implementation graceful if the module is missing or wiring is wrong.
- Add local guard scripts for menu integration and NRF feature structure before hardware testing.
- A second NRF24L01 node will be needed to fully prove send/receive behavior.

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
