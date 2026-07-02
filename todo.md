# Scoober Todo

Prioritized next tasks for the project. Keep this file current so a new Codex session can continue without relying on chat history.

## Priority 1: Stabilize Current Hardware Features

- Test Environment with ENV III connected at boot.
- Test Environment by opening the screen first, then plugging in ENV III and waiting for retry.
- Test Environment with ENV III unplugged to confirm the firmware stays usable.
- Confirm keyboard navigation still works while ENV III is plugged into Grove.
- Confirm Voice Memos still records and plays after adding M5Unit-ENV.
- Confirm Battery trend still behaves when charging starts/stops outside the Battery screen.

## Priority 2: Environment Feature Improvements

- Add optional environment logging to microSD as CSV.
- Decide CSV columns before coding. Suggested columns: uptime seconds, temperature C, temperature F, humidity percent, pressure hPa, altitude m.
- Add a start/stop logging control instead of logging automatically.
- Show log file name and sample count on the Environment screen.
- Add graceful behavior when SD is missing while Environment logging is requested.
- Consider units/settings: C/F toggle, pressure hPa/inHg toggle, refresh interval.

## Priority 3: Project Structure Cleanup

- Split `src/main.cpp` into smaller feature files once current behavior is stable.
- Suggested future files:
  - `src/ui.h` / `src/ui.cpp`
  - `src/power_screen.h` / `src/power_screen.cpp`
  - `src/wifi_screens.h` / `src/wifi_screens.cpp`
  - `src/voice_memos.h` / `src/voice_memos.cpp`
  - `src/environment_screen.h` / `src/environment_screen.cpp`
  - `src/level_tool.h` / `src/level_tool.cpp`
- Only refactor after guard scripts are passing and the user is not actively testing a hardware issue.

## Priority 4: Voice Memo Improvements

- Add a clearer playback progress indicator.
- Add a cancel playback key if practical.
- Add memo duration display after scanning WAV headers.
- Decide whether recordings should be named by timestamp once a clock/time source exists.
- Consider lowering speaker volume or making volume configurable.

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
- Added scrollable WiFi Scan.
- Added persistent Saved WiFi names with delete flow.
- Added Voice Memos to microSD.
- Added Level tool using IMU crosshair/dot display.
- Tried and removed active OLED support.
- Added Environment screen for M5Stack ENV III Unit.
