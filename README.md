# Scoober

## Project Overview And Goals

Scoober is custom firmware for the **M5Stack Cardputer Adv Version**. It started as a safe starter firmware called `cardputer-project-launcher` and has evolved into a small, working Cardputer utility hub.

The current goal is not to build the final product in one leap. The goal is to keep a stable firmware foundation that proves custom development on the Cardputer Adv, then add useful features one at a time without breaking the keyboard, display, battery behavior, SD card, or Grove expansion path.

## Quick Start For Next Codex Session

Start here if this repository is opened in a fresh Codex chat.

1. Read this file, then read [notes.md](notes.md) and [todo.md](todo.md).
2. Do not assume unused pins are safe. **Avoid using G8/G9 directly** for external I2C hardware on the Cardputer Adv; those pins share the internal I2C bus and caused keyboard failures during OLED testing.
3. Priority #5 now includes a hardware-tested WiFi Connect screen that reads credentials from microSD `/config/wifi.txt`. Do not hardcode credentials, and keep `WiFi.begin` limited to that intentional flow.
4. Priority #6 now has a hardware-tested MQTT `Pi Monitor` screen using Raspberry Pi settings from microSD `/config/pi.txt`, hardware-tested Cardputer status/availability publishing, hardware-tested `read_now` and `set_interval` commands, and hardware-tested target selection.
5. Priority #7 is using the M5Stack Unit PaHub v2.1 I2C expansion path. ENV III remains on PaHub channel 0, and the SSD1309 OLED remains on PaHub channel 1 as a small OLED Status Dashboard plus diagnostics screen.
6. Do not revive ESP-NOW RC controller work. The user decided this device is not going to be the RC controller.
7. NRF24L01 feature was removed from the active Cardputer firmware on 2026-08-25 so the EXT path is available for Cap LoRa-1262 planning.
8. Priority #10 now has an RX-only `LoRa Diag` screen for the M5Stack Cap LoRa-1262. No transmit path is enabled.
9. Do not reopen the retired XIAO NRF24 two-node debugging path unless the user explicitly asks; it is historical only.
10. Build with `python -m platformio run` if `pio` is not on PATH.
11. Run all guard scripts before claiming work is complete:

```sh
python tools/check_battery_trend.py
python tools/check_display_refresh.py
python tools/check_environment_feature.py
python tools/check_external_display_revisit.py
python tools/check_level_tool.py
python tools/check_lora_cap_diag.py
python tools/check_menu_structure.py
python tools/check_nrf24_feature.py
python tools/check_oled_test.py
python tools/check_oled_status_dashboard.py
python tools/check_power_status.py
python tools/check_pi_command_center_plan.py
python tools/check_saved_wifi.py
python tools/check_wifi_credentials_strategy.py
python tools/check_voice_memos.py
python tools/check_wifi_scroll.py
python tools/check_xiao_nrf24_node.py
python -m platformio run
```

## Current Project Status

Current firmware name: **Scoober**

Current firmware version in code: `v0.1.0`

Current state:

- Builds successfully with PlatformIO / Arduino for `m5stack-stamps3`.
- Boots to a splash screen showing `M5 Cardputer Lab`, `Scoober`, and firmware version.
- Uses arrow keys and OK/Enter for menu navigation.
- Uses Backspace as the return-to-menu key from feature screens.
- Uses the built-in display as the main control display.
- Uses the SSD1309 OLED as a small secondary status display.
- Uses the Grove I2C port for the M5Stack ENV III Unit, either directly or through M5Stack Unit PaHub v2.1 channel 0.
- Uses microSD for voice memo storage.
- Has no active NRF24L01/RF Scan feature or RF24 dependency in the main Cardputer firmware.
- Has an RX-only Cap LoRa-1262 diagnostics screen with graceful `LoRa not found` status and separate GNSS UART byte/line counters.
- Has a GNSS parser for Cap LoRa-1262 using TinyGPSPlus.
- User confirmed Cap LoRa-1262 diagnostics are working on hardware on 2026-08-25.
- Has a hardware-tested WiFi Connect screen that reads `/config/wifi.txt` from microSD and never stores Wi-Fi passwords in source code or NVS.
- Has a hardware-tested Pi Monitor screen that reads `/config/pi.txt`, connects to MQTT, subscribes to Raspberry Pi home IoT device topics, publishes Cardputer status/availability, and publishes whitelisted MQTT commands.
- Has no active ESP-NOW code.
- Has an OLED Status Dashboard for the SSD1309 OLED on PaHub channel 1.
- Keeps OLED Test as a diagnostics/proof-of-life screen.
- User confirmed OLED Test works on hardware on 2026-08-15.
- User confirmed the OLED Status Dashboard is working across the tested features on 2026-08-22.

## Hardware Being Used

Primary device:

- M5Stack Cardputer Adv Version
- MCU: ESP32-S3 / Stamp-S3A style target
- Built-in display: 1.14 inch 240x135 ST7789 LCD
- Built-in keyboard
- Built-in battery/power system
- Built-in microSD card slot
- Built-in BMI270 IMU on the Adv version
- Built-in Wi-Fi/BLE from ESP32-S3
- Built-in microphone and speaker
- Grove port

External module currently supported:

- M5Stack Unit PaHub v2.1
  - PCA9548AP I2C multiplexer
  - Default I2C address: `0x70`
  - Firmware uses a tiny direct selector helper; no extra PaHub library is required yet
- M5Stack ENV III Unit
  - Direct Grove or PaHub channel 0
- Sensors used through `M5Unit-ENV`:
  - SHT30 for temperature and humidity
  - QMP6988 for air pressure

Optional hardware:

- microSD card for Voice Memos
- USB-C data cable for upload and serial monitor
- SSD1309 OLED on PaHub channel 1 for the OLED Status Dashboard and OLED Test diagnostics screen
- Planned Priority #9 hardware: DS3231 / AT24C32 I2C RTC module
- M5Stack Cap LoRa-1262 for Cardputer Adv, diagnostics only

Hardware intentionally not active right now:

- ESP-NOW RC controller hardware
- Direct Raspberry Pi shell/admin control; Pi Monitor stays scoped to MQTT monitoring plus whitelisted JSON commands
- NRF24L01 / RF Scan firmware feature; removed to avoid conflicts with the upcoming Cap LoRa-1262 EXT interface path
- DS3231 / AT24C32 RTC module; planned for Priority #9
- Cap LoRa-1262 transmit features; RX diagnostics are active, but No transmit work happens until antenna, legal region/frequency, bandwidth/spreading plan, and TX power are deliberately set
- IR, BLE, audio beyond voice memos, and other expansion hardware

Current external-display direction: keep the built-in LCD as the primary control UI, and use the SSD1309 OLED as a small glance/status display on the M5Stack Unit PaHub v2.1. ENV III remains on PaHub channel 0, SSD1309 OLED remains on PaHub channel 1, PaHub default address `0x70`. Priority #8 is to customize each feature's OLED status. Avoid using G8/G9 directly on the Cardputer Adv because those pins share the internal I2C bus with the keyboard.

Next planned hardware:

- Priority #9: DS3231 / AT24C32 I2C RTC module.
  - Likely future use: timestamps for logs, voice memo file names, OLED clock/status, and Pi Monitor events.
  - Must use a documented safe I2C path, likely another PaHub channel after review.
- Priority #10: M5Stack Cap LoRa-1262 for Cardputer Adv.
  - Hardware includes SX1262 LoRa and ATGM336H GNSS.
  - It connects through the Cardputer Adv EXT interface.
  - The old NRF24/RF Scan firmware path has been removed so LoRa/GNSS diagnostics can claim the EXT path cleanly.
  - First milestone is active as `LoRa Diag`: detect the PI4IOE5V6408 antenna-switch expander, initialize SX1262 with RadioLib in receive-only mode, and count GNSS UART bytes/NMEA lines.
  - GNSS parser milestone uses TinyGPSPlus to show fix status, satellites, HDOP, coordinates, UTC time, and checksum counters.
  - Do not transmit until antenna, region/frequency, and TX power are deliberately set.

## Software, Libraries, And Frameworks

Development environment:

- PlatformIO
- Arduino framework
- VS Code / PlatformIO extension recommended

PlatformIO environment:

- `default_envs = m5stack-stamps3`
- `platform = espressif32`
- `board = m5stack-stamps3`
- `framework = arduino`
- `monitor_speed = 115200`
- `upload_speed = 1500000`

Libraries in [platformio.ini](platformio.ini):

- `m5stack/M5Cardputer`
- `m5stack/M5Unit-ENV`
- `z3t0/IRremote@^4.7.1`
- `knolleary/PubSubClient@^2.8`
- `bblanchon/ArduinoJson@^6.21.5`
- `olikraus/U8g2@^2.36.12`
- `jgromes/RadioLib`
- `https://github.com/m5stack/TinyGPSPlus.git`

RF24 dependency is not used by the main firmware.

Arduino/core libraries used by the firmware:

- `M5Cardputer.h`
- `M5UnitENV.h`
- `Preferences.h`
- `SPI.h`
- `SD.h`
- `Wire.h`
- `WiFi.h`
- `PubSubClient.h`
- `ArduinoJson.h`
- `RadioLib.h`
- `TinyGPSPlus.h`
- `math.h`

Important build note:

- In the Codex shell, `pio` may not be on PATH.
- `python -m platformio run` has been used successfully.
- `python -m platformio run --target upload` is the equivalent upload command.

## Folder Structure

```text
.
|-- README.md
|-- notes.md
|-- todo.md
|-- platformio.ini
|-- src/
|   |-- app.h
|   |-- app_state.cpp
|   |-- environment_screen.cpp
|   |-- i2c_hub.cpp
|   |-- input.cpp
|   |-- level_tool.cpp
|   |-- main.cpp
|   |-- oled_test.cpp
|   |-- pi_monitor.cpp
|   |-- power_screen.cpp
|   |-- ui.cpp
|   |-- voice_memos.cpp
|   |-- wifi_connect.cpp
|   `-- wifi_screens.cpp
|-- nodes/
|   `-- xiao_nrf24_oled/
|       |-- README.md
|       |-- platformio.ini
|       `-- src/
|           `-- main.cpp
|-- tools/
|   |-- check_battery_trend.py
|   |-- check_display_refresh.py
|   |-- check_environment_feature.py
|   |-- check_level_tool.py
|   |-- check_menu_structure.py
|   |-- check_nrf24_feature.py
|   |-- check_oled_test.py
|   |-- check_oled_status_dashboard.py
|   |-- check_pi_command_center_plan.py
|   |-- check_power_status.py
|   |-- check_saved_wifi.py
|   |-- check_wifi_credentials_strategy.py
|   |-- check_voice_memos.py
|   |-- check_wifi_scroll.py
|   |-- firmware_source.py
|   `-- check_xiao_nrf24_node.py
`-- docs/
    `-- superpowers/
        `-- plans/
            |-- 2026-06-27-saved-wifi-feedback-delete.md
            |-- 2026-06-27-saved-wifi-names.md
            |-- 2026-06-28-voice-memos.md
            |-- 2026-07-01-environment-feature.md
            `-- 2026-07-25-raspberry-pi-command-center.md
```

File responsibilities:

- [src/main.cpp](src/main.cpp): firmware entry point with `setup()` and `loop()`.
- [src/app.h](src/app.h): shared constants, types, global declarations, and function prototypes.
- [src/app_state.cpp](src/app_state.cpp): shared global state, menu definitions, and hardware helper objects.
- [src/ui.cpp](src/ui.cpp): screen routing, header/content drawing helpers, and main menu rendering.
- [src/input.cpp](src/input.cpp): keyboard event dispatch and screen-specific key handling.
- [src/power_screen.cpp](src/power_screen.cpp): Battery/System screens and battery trend logic.
- [src/wifi_connect.cpp](src/wifi_connect.cpp): SD-backed Wi-Fi credential reading and connect/disconnect screen.
- [src/wifi_screens.cpp](src/wifi_screens.cpp): Wi-Fi scan, saved SSID list, save/delete flows, and Preferences storage.
- [src/pi_monitor.cpp](src/pi_monitor.cpp): SD-backed Raspberry Pi MQTT config, subscribe flow, compact device monitor screen, Cardputer status/availability publisher, and whitelisted command publishers.
- [src/voice_memos.cpp](src/voice_memos.cpp): microSD WAV recording, listing, playback, and delete flow.
- [src/i2c_hub.cpp](src/i2c_hub.cpp): optional M5Stack Unit PaHub v2.1 detection and channel selection for shared Grove I2C.
- [src/environment_screen.cpp](src/environment_screen.cpp): ENV III sensor readings and CSV logging.
- [src/oled_test.cpp](src/oled_test.cpp): SSD1309 OLED Status Dashboard plus OLED Test diagnostics on PaHub channel 1.
- [src/lora_diag.cpp](src/lora_diag.cpp): RX-only M5Stack Cap LoRa-1262 diagnostics using RadioLib for SX1262 and TinyGPSPlus for parsed GNSS status.
- [src/level_tool.cpp](src/level_tool.cpp): BMI270 level/crosshair tool.
- [platformio.ini](platformio.ini): board/framework/library configuration.
- [nodes/xiao_nrf24_oled](nodes/xiao_nrf24_oled): archived separate PlatformIO project for the XIAO ESP32-C3 NRF24/OLED proof node.
- [tools/](tools): lightweight Python guard scripts. These are not full unit tests, but they catch accidental removal of important behavior and design decisions. Firmware guards use [tools/firmware_source.py](tools/firmware_source.py) to scan all `src` files.
- [docs/superpowers/plans/](docs/superpowers/plans): implementation plans, planning notes, and completed feature history.
- [docs/superpowers/plans/2026-07-25-raspberry-pi-command-center.md](docs/superpowers/plans/2026-07-25-raspberry-pi-command-center.md): Priority #6 MQTT Pi Monitor planning.
- [notes.md](notes.md): deeper technical notes, lessons learned, and reasoning.
- [todo.md](todo.md): prioritized next work.

## Current Working Features

### Main Menu

Menu items:

1. Battery
2. System
3. WiFi Scan
4. Saved WiFi
5. WiFi Connect
6. Pi Monitor
7. Voice Memos
8. Environment
9. OLED Test
10. LoRa Diag
11. Level

Navigation:

- Arrow keys are represented in the current firmware by keyboard characters:
  - `;` or `,` moves up
  - `.` or `/` moves down
- OK/Enter opens the selected feature.
- Backspace returns to the main menu from most feature screens.
- The menu shows eight rows at a time and scrolls when the selected item moves beyond the visible rows.
- The old bottom footer text was removed from feature screens by user request.

### Battery

Shows:

- Battery voltage in mV
- Battery percentage when available
- Charging estimate: `Charging` or `Not charging`
- Raw M5 power API status
- VBUS voltage when available
- Battery current when available
- Voltage/percentage trend

Important battery behavior:

- The Cardputer power API does not always provide reliable charging/current status.
- Charging detection uses trend logic rather than trusting the API alone.
- The trend is sampled continuously in `loop()`, even while the user is not inside the Battery feature.
- A deadzone is used: charging requires voltage trend above `CHARGING_TREND_THRESHOLD_MV = 50`.
- Confirmation requires `CHARGING_CONFIRM_SAMPLES = 3`.
- Charging also requires VBUS/external power above `VBUS_PRESENT_THRESHOLD_MV = 4500`.
- A confirmed drop from the sampled voltage peak clears inferred charging and restarts the trend baseline.
- Battery percentage is displayed but not used as charging evidence because it can jump with voltage-based estimation.
- This was tuned after unplugged fluctuations around +/-10 mV, then an observed unplugged +26 mV drift, caused false charging indicators, while real charging trends were around +100 mV.

### System

Shows:

- Firmware version
- Uptime in seconds
- Free heap
- CPU frequency
- Flash size
- Screen dimensions

Firmware version was intentionally removed from Battery and left in System only.

### WiFi Scan

Behavior:

- Scans nearby Wi-Fi networks.
- Does not require or store credentials.
- Does not connect to Wi-Fi.
- Displays a scrollable list of up to `MAX_WIFI_NETWORKS = 40`.
- Shows up to `WIFI_VISIBLE_ROWS = 5` rows at a time.
- OK/Enter opens a save confirmation for the selected SSID.
- `R` rescans.
- Full scan results are printed to Serial.

Control summary:

- Use arrow keys to scroll.
- Press OK/Enter to save the selected network name.
- Press R to rescan.

### Saved WiFi

Behavior:

- Stores only SSID names.
- Uses ESP32 Preferences / NVS namespace `scoober_wifi`.
- Maximum saved names: `MAX_SAVED_WIFI_NAMES = 20`.
- Saved names survive reboot and power-off.
- Duplicate names are rejected.
- Hidden/empty SSIDs cannot be saved.
- `D` opens delete confirmation for the selected saved SSID.

This feature intentionally does not store passwords and intentionally does not call `WiFi.begin`.

### WiFi Connect

Behavior:

- Reads Wi-Fi credentials from a microSD card file.
- Does not hardcode credentials in source code.
- Does not store Wi-Fi passwords in Preferences/NVS.
- Shows missing SD, missing config, missing SSID, connecting, connected, failed, and disconnected status.
- Uses a 15 second connection timeout: `WIFI_CONNECT_TIMEOUT_MS = 15000`.
- Shows the connected IP address when available.
- OK/Enter retries the connection.
- `D` disconnects from Wi-Fi.

Credential file path on the Cardputer microSD card:

```text
/config/wifi.txt
```

Credential file format:

```text
ssid=YourNetworkName
password=YourNetworkPassword
```

Rules for the WiFi Connect feature:

- Do not hardcode credentials in source code.
- Do not store Wi-Fi passwords in Preferences/NVS.
- Do not commit real `wifi.txt` files. This repo ignores `/config/wifi.txt` and `/wifi.txt` in case local copies are created while testing.
- Keep the existing Saved WiFi feature as SSID-only unless the user explicitly asks to change it.
- `WiFi.begin` belongs only in the intentional WiFi Connect flow.
- Connection attempts show clear status, use a timeout, handle missing SD/config gracefully, and return safely to the menu.

### Pi Monitor

Behavior:

- Raspberry Pi MQTT monitor with two whitelisted command actions.
- Requires Wi-Fi to already be connected through WiFi Connect.
- Reads Raspberry Pi MQTT settings from a microSD card file.
- Connects to the MQTT broker with `PubSubClient`.
- Subscribes to `home/#` so terminal-published test messages are visible.
- Treats `home/devices/<device>/<kind>` topics as structured device-list updates.
- Publishes retained Cardputer availability to `home/devices/scoober-cardputer/availability`.
- Publishes retained Cardputer status JSON to `home/devices/scoober-cardputer/status` after MQTT connect and about once per minute while Pi Monitor is open.
- Uses an MQTT last-will so unexpected disconnects can mark Cardputer availability as `offline`.
- User confirmed Cardputer MQTT status/availability publishing works on hardware on 2026-08-15.
- Displays MQTT status, broker address, message count, last topic/payload, command response display, and a compact device list.
- Displays the most recent command status on the monitor screen.
- Shows `home/devices/<device>/responses`, nested response topics, or post-command updates from `command_target` on a clearer `Resp:` line.
- Current caveat: user hardware testing on 2026-08-06 still showed `Resp: waiting.`, so `Last` / `Pay` remain the reliable command-feedback view for now.
- Parses small JSON status/telemetry payloads with ArduinoJson.
- `C` publishes `{"command":"read_now"}` to `home/devices/<command_target>/commands`.
- `T` cycles the command target through `command_target` and discovered device IDs.
- `I` cycles fixed `set_interval` choices: 10, 30, 60, and 300 seconds.
- `S` publishes `{"command":"set_interval","seconds":<selected>}` to `home/devices/<command_target>/commands`.
- Does not publish arbitrary command text, run shell actions, or perform Raspberry Pi admin actions.
- Disconnects MQTT when leaving the screen.
- OK/Enter retries MQTT connection.
- `R` clears the device list and reconnects.
- `D` disconnects MQTT.

Pi Monitor config file path on the Cardputer microSD card:

```text
/config/pi.txt
```

Config file format:

```text
mqtt_host=10.0.0.180
mqtt_port=1883
device_id=scoober-cardputer
command_target=esp32-c3-test
```

Rules for Pi Monitor:

- Reuse WiFi Connect for Wi-Fi; do not add another Wi-Fi credential path.
- Do not hardcode Pi IPs, MQTT settings, Wi-Fi credentials, or MQTT passwords in source.
- Do not commit real `pi.txt` files. This repo ignores `/config/pi.txt` and `/pi.txt` in case local copies are created while testing.
- Keep command publishing whitelisted and explicit. The allowed commands are `read_now` and fixed-choice `set_interval` to `home/devices/<command_target>/commands`.
- Do not add free-form interval entry; keep `set_interval` seconds limited to firmware-defined choices.
- Cardputer may publish only scoped identity/status topics for its own configured `device_id`.
- User confirmed Pi Monitor MQTT viewing works on Cardputer hardware on 2026-07-29.
- User confirmed Pi Monitor `read_now` command publishing works on Cardputer hardware on 2026-07-30.

### Voice Memos

Behavior:

- Uses built-in microphone and speaker.
- Requires microSD card.
- Stores files in `/memos`.
- File names are `memo001.wav` through `memo999.wav`.
- Records 16-bit mono WAV files.
- Sample rate: `VOICE_RECORD_SAMPLE_RATE = 16000`.
- Max recording duration: `VOICE_RECORD_MAX_SECONDS = 30`.
- Record chunk samples: `VOICE_RECORD_CHUNK_SAMPLES = 240`.
- Max listed memos: `MAX_VOICE_MEMOS = 30`.
- `R` starts/stops recording.
- OK/Enter plays selected memo.
- `D` deletes selected memo after confirmation.

Control summary:

- Press R to start or stop recording.
- Press OK/Enter to play the selected memo.
- Press D to delete the selected memo after confirmation.

SD card pins:

- SCK: `G40`
- MISO: `G39`
- MOSI: `G14`
- CS: `G12`
- SPI frequency: `25000000`

Voice memo notes:

- Playback is currently a simple blocking playback loop. It calls `M5Cardputer.update()` while waiting for the speaker, but the UI is not a full asynchronous audio player.
- Recording stops automatically at 30 seconds.
- If zero bytes are recorded, the file is removed and no audio is saved.

### Environment

Behavior:

- Reads the M5Stack ENV III Unit over Grove I2C.
- Supports direct Grove wiring and the M5Stack Unit PaHub v2.1 path.
- Detects the PaHub at default address `0x70`.
- Selects ENV III on PaHub channel 0 before sensor initialization and reads.
- Waits briefly after PaHub channel selection before talking to the ENV III sensors.
- Falls back to direct Grove behavior when no PaHub is detected.
- Uses the global Arduino `Wire` object for ENV III because this path reads both SHT30 and QMP6988 correctly on the Cardputer Adv.
- Shows:
  - Temperature in C and F
  - Humidity in percent
  - Pressure in hPa
  - Sensor status
- Filters out impossible pressure values instead of showing misleading readings.
- Pressure must be finite and between `300.0` and `1100.0` hPa to display or log.
- If QMP6988 returns a bad value, the screen shows `Pressure: invalid`.
- Invalid pressure retries sensor initialization periodically in case the QMP6988 calibration read was bad.
- Optional CSV logging to microSD.
- Refreshes about once per second.
- If the unit is unplugged or not found, the screen shows `ENV III not found`, shows the active I2C path, and retries every few seconds.
- Supports partial sensor availability. If only SHT30 or only QMP6988 responds, the screen can still show the readings it can get.
- Press `L` to name and start logging; press `L` again while logging to stop.
- The log-name screen accepts letters, numbers, spaces, `_`, and `-`.
- Backspace deletes characters while naming; OK/Enter starts logging.
- Blank names fall back to `/env/env001.csv`.
- Named sessions create files such as `/env/backyard001.csv`; spaces become underscores and names are capped at 16 characters.
- CSV columns: `uptime_s,temp_c,temp_f,humidity_pct,pressure_hpa`.
- The Environment screen shows the log file name and sample count while logging.
- If the SD card is missing, logging shows an error and the firmware remains usable.

ENV constants:

- SDA: `ENV_I2C_SDA_PIN = 2`
- SCL: `ENV_I2C_SCL_PIN = 1`
- I2C frequency: `ENV_I2C_FREQUENCY = 400000`
- PaHub address: `I2C_HUB_ADDRESS = 0x70`
- ENV III PaHub channel: `I2C_HUB_ENV_CHANNEL = 0`
- OLED PaHub channel: `I2C_HUB_OLED_CHANNEL = 1`
- PaHub settle delay: `I2C_HUB_CHANNEL_SETTLE_US = 1000`
- Pressure sanity range: `300.0` to `1100.0` hPa
- Refresh: `ENV_REFRESH_INTERVAL_MS = 1000`
- Retry: `ENV_RETRY_INTERVAL_MS = 3000`

ENV wiring, direct Grove:

- Easiest: plug ENV III into the Cardputer Grove port.
- If wiring manually:
  - VCC -> Grove 5V
  - GND -> Grove GND
  - SDA -> Grove SDA
  - SCL -> Grove SCL

ENV wiring, PaHub path:

- Plug the Unit PaHub v2.1 input into the Cardputer Grove port.
- Leave the PaHub DIP switch at the default address `0x70` for the first test.
- Plug ENV III into PaHub channel 0.
- Plug the SSD1309 OLED into PaHub channel 1 for the OLED Status Dashboard and OLED Test diagnostics.
- Do not connect the OLED to G8/G9.

### OLED Status Dashboard And Test

Behavior:

- Adds a compact OLED Status Dashboard for the SSD1309 OLED.
- Keeps `OLED Test` as a diagnostics/proof-of-life screen.
- Uses U8g2 with `U8G2_SSD1309_128X64_NONAME0_F_HW_I2C`.
- Requires the M5Stack Unit PaHub v2.1 at default address `0x70`.
- OLED remains on PaHub channel 1.
- ENV III remains on PaHub channel 0.
- Selects PaHub channel 1 before probing, initializing, and drawing OLED content.
- Probes OLED I2C addresses `0x3C` and `0x3D`.
- The built-in Cardputer LCD remains the main control screen.
- The OLED is a small glance/status display, not a duplicate of the built-in screen.
- Dashboard drawing is centralized in `renderOledStatusDashboard()`.
- `serviceOledStatusDashboard()` refreshes the OLED about once per second from `loop()`.
- Features can later provide a short context line through `setOledStatusLine(...)` without knowing U8g2 details.
- Missing PaHub or missing OLED shows a status on the built-in OLED Test screen and keeps firmware usable.
- OLED detection retries periodically when the dashboard is active.
- Do not connect the OLED to G8/G9.

Dashboard content:

- Current screen/mode.
- Battery level or charging status.
- Wi-Fi connected/disconnected.
- MQTT connected/disconnected when Pi Monitor is active or has been used.
- A compact context line for the active feature.

Screen-specific examples:

- Main menu: `Scoober`, battery, Wi-Fi, mode/menu selection.
- Pi Monitor: MQTT status, selected target, command status, message/device counts.
- Environment: temperature F, humidity, pressure value or `Press: invalid`.
- Voice Memos: `REC mm:ss` while recording, active/selected memo, SD/status.
- Level: compact level/tilt context.

OLED Test diagnostics:

- Draws `Scoober OLED`, `SSD1309 ch1`, the active address, a draw counter, a border, and a moving marker.
- OK/Enter or R retries OLED detection.
- Backspace returns to the main menu.

OLED constants:

- PaHub channel: `I2C_HUB_OLED_CHANNEL = 1`
- Primary OLED address: `OLED_I2C_ADDRESS_PRIMARY = 0x3C`
- Secondary OLED address: `OLED_I2C_ADDRESS_SECONDARY = 0x3D`
- OLED Test refresh interval: `OLED_TEST_REFRESH_INTERVAL_MS = 1000`
- OLED Status Dashboard refresh interval: `OLED_STATUS_REFRESH_INTERVAL_MS = 1000`
- OLED retry interval: `OLED_RETRY_INTERVAL_MS = 3000`
- OLED status lines: `OLED_STATUS_LINE_COUNT = 5`
- OLED line length: `OLED_STATUS_MAX_CHARS = 21`

### Cap LoRa-1262 Diagnostics

Behavior:

- Adds `LoRa Diag` as the first Priority #10 firmware milestone.
- Uses RadioLib for the SX1262 LoRa radio.
- Detects the Cap LoRa-1262 PI4IOE5V6408 I/O expander at `0x43`.
- Enables the required antenna switch by setting PI4IOE5V6408 `P0` high.
- Initializes SX1262 receive mode only and shows `LoRa not found` instead of crashing when the cap or radio is absent.
- Starts the ATGM336H GNSS serial path separately at `115200` 8N1 and counts bytes/NMEA lines.
- Adds a GNSS parser using TinyGPSPlus.
- Shows packet count, live channel RSSI before the first packet, packet RSSI/SNR after a matching LoRa packet, GNSS byte/line counts, and the latest clipped NMEA line on the built-in LCD.
- Shows parsed GNSS fix/no-fix, satellites, HDOP, latitude, longitude, UTC time, sentence count, and checksum errors.
- SNR is packet-only; it shows `--pkt` until a LoRa packet is received.
- User confirmed the screen reports cap/RF/radio status, live RSSI, and GNSS NMEA output on hardware on 2026-08-25.
- Adds compact LoRa status lines to the SSD1309 OLED Status Dashboard.
- OK/Enter or R restarts diagnostics.
- Backspace returns to the main menu and stops the radio/GNSS diagnostics objects.
- No transmit path is enabled in this milestone.

Cap LoRa-1262 pin map documented from M5Stack docs:

- `G5 NSS`
- `G4 IRQ`
- `G3 RST`
- `G6 BUSY`
- `G40 SCK`
- `G14 MOSI`
- `G39 MISO`
- `G15 GPS-TX` to Cardputer RX
- `G13 GPS-RX` from Cardputer TX
- `G8/G9` internal I2C for the cap PI4IOE5V6408 and HY2.0-4P path
- PI4IOE5V6408 `P0` controls the SX1262 antenna switch

Shared-pin conflict notes:

- LoRa SPI uses `G40/G39/G14`, the same SPI signal pins used by microSD.
- LoRa `NSS` is `G5`; microSD `CS` remains `G12`.
- `LoRa Diag` explicitly drives the microSD CS pin high before initializing the SX1262 so both SPI devices are not selected together.
- The firmware only services LoRa/GNSS while the `LoRa Diag` screen is active.
- Do not use G8/G9 directly for external OLED/ENV hardware; on this cap they are the internal I2C path required by the cap expander.
- No transmit work should be added until the antenna is installed and the legal region/frequency, bandwidth/spreading plan, and TX power are intentionally chosen.

LoRa constants:

- `LORA_NSS_PIN = 5`
- `LORA_IRQ_PIN = 4`
- `LORA_RST_PIN = 3`
- `LORA_BUSY_PIN = 6`
- `LORA_SPI_SCK_PIN = 40`
- `LORA_SPI_MOSI_PIN = 14`
- `LORA_SPI_MISO_PIN = 39`
- `LORA_GNSS_RX_PIN = 15`
- `LORA_GNSS_TX_PIN = 13`
- `LORA_GNSS_BAUD = 115200`
- `LORA_IO_EXPANDER_ADDRESS = 0x43`
- `LORA_RF_SWITCH_PIN = 0`
- `LORA_DIAG_RX_FREQUENCY_MHZ = 915.0`
- `LORA_DIAG_BANDWIDTH_KHZ = 125.0`
- `LORA_DIAG_SPREADING_FACTOR = 12`
- `LORA_DIAG_CODING_RATE = 5`
- `LORA_DIAG_SYNC_WORD = 0x34`
- `LORA_DIAG_PREAMBLE_LEN = 20`
- `LORA_GNSS_FIX_STALE_MS = 5000`

### NRF24L01 / RF Scan Removal

The active Cardputer RF Scan screen and NRF24L01 module support were removed on 2026-08-25. The NRF24 path used the Cardputer Adv EXT SPI pins, which conflicts with the upcoming M5Stack Cap LoRa-1262 diagnostics and GNSS work.

Removal scope:

- No RF Scan main-menu item.
- No `Screen::RfScanner` route, keyboard handling, or OLED dashboard page.
- No `src/rf_scanner.cpp` in active firmware.
- No RF24 dependency in the main `platformio.ini`.
- `tools/check_nrf24_feature.py` now guards that the active Cardputer firmware stays free of NRF24/RF Scan code.

### Archived XIAO NRF24 OLED Node

The second proof node lives in [nodes/xiao_nrf24_oled](nodes/xiao_nrf24_oled).

This node is kept for future investigation, but it is no longer the active Cardputer feature.
Hardware testing showed:

- Cardputer pings reached the XIAO node.
- XIAO `TX` and `OK` counts increased and `F` stayed at 0.
- Cardputer did not receive `XIAO beacon N` or `XIAO ack N`.
- Cardputer RF RPD/carrier counters moved during ping tests, but valid packets did not decode.
- The likely next bench steps are swapping NRF24 modules, improving PA/LNA module power, adding a capacitor near the radio, or using a known-good third node.

Hardware:

- Seeed Studio XIAO ESP32-C3
- Same style NRF24L01+ PA+LNA module and adapter/breakout
- 0.96 inch SSD1306 128x64 I2C OLED

XIAO OLED wiring:

- SDA -> XIAO GPIO6 / SDA
- SCL -> XIAO GPIO7 / SCL
- VCC -> 3V3 or the OLED module's required VCC
- GND -> GND

XIAO NRF24 wiring:

- CE -> XIAO D7 / GPIO20
- CSN -> XIAO D8 / GPIO8
- SCK -> XIAO D9 / GPIO9
- MOSI -> XIAO D10 / GPIO10
- MISO -> XIAO D6 / GPIO21
- GND -> GND
- VCC -> the NRF24 adapter/breakout's required VCC

XIAO node behavior:

- Uses the same shared address as the Cardputer: `SCBR1`.
- Uses `RF24_250KBPS` for the archived proof protocol.
- Shows TX attempts, TX OK/fail counts, RX pipe, FIFO state, and last packet text on the SSD1306 display.
- Sends a `XIAO beacon N` packet about four times per second.
- Uses `RF24_PA_MIN` on the XIAO side to reduce PA/LNA transmit current while testing close range.
- Replies to Cardputer pings with three `XIAO ack N` packets.
- Waits briefly before replying so the Cardputer has time to return to listen mode.

Build the XIAO node from the repo root:

```sh
python -m platformio run -d nodes/xiao_nrf24_oled
```

Upload the XIAO node from the repo root:

```sh
python -m platformio run -d nodes/xiao_nrf24_oled --target upload
```

### Level

Behavior:

- Uses the Cardputer Adv BMI270 IMU through M5Unified.
- Replaced the earlier simple IMU Test.
- Draws a crosshair and moving dot.
- When the dot aligns with the center tolerance, the screen shows `LEVEL`.
- Also shows pitch and roll.

Level constants:

- `LEVEL_DOT_SCALE_PIXELS = 70.0f`
- `LEVEL_TOLERANCE_PIXELS = 6`
- `LEVEL_SMOOTHING = 0.25f`

## Pin Mappings And Wiring Details

Known pins used by current firmware:

| Purpose | Pin(s) | Notes |
| --- | --- | --- |
| microSD SCK | G40 | Used by Voice Memos |
| microSD MISO | G39 | Used by Voice Memos |
| microSD MOSI | G14 | Used by Voice Memos |
| microSD CS | G12 | Used by Voice Memos |
| ENV III SDA | G2 / Grove SDA | External Grove I2C |
| ENV III SCL | G1 / Grove SCL | External Grove I2C |
| Unit PaHub v2.1 input | G2/G1 Grove | Optional external I2C multiplexer |
| Unit PaHub v2.1 address | 0x70 | Default DIP-switch address |
| ENV III via PaHub | Channel 0 | First Priority #7 hardware test |
| SSD1309 OLED via PaHub | Channel 1 | OLED Status Dashboard and OLED Test diagnostics |
| Cap LoRa-1262 SX1262 SPI | G40/G39/G14, G5 NSS | Shared EXT SPI signal pins with microSD; separate chip select |
| Cap LoRa-1262 SX1262 control | G4 IRQ, G3 RST, G6 BUSY | Used only by `LoRa Diag` |
| Cap LoRa-1262 GNSS UART | G15 GPS-TX, G13 GPS-RX | Cardputer RX is G15; Cardputer TX is G13 |
| Cap LoRa-1262 antenna switch | PI4IOE5V6408 P0 | I/O expander at `0x43` on internal G8/G9 I2C |
| Internal Cardputer I2C | G8/G9 | Do not use directly for external I2C modules |
| Download mode | G0 | Hold while applying USB/power if upload fails |

Important I2C note:

- On the Cardputer Adv, `G8/G9` are used by the internal I2C bus.
- That internal bus includes the keyboard path. Attaching an external OLED to G8/G9 caused arrow keys to stop working.
- Keep external I2C modules on Grove `G2/G1` unless there is a deliberate expansion/mux/buffer plan.

## Communication Architecture

Current active communication paths:

- **USB Serial**
  - Baud: `115200`
  - Used for boot messages, keyboard events, Wi-Fi scan results, voice memo events, and environment readings.
- **Wi-Fi**
  - Used for station-mode scanning.
  - Can connect through the WiFi Connect screen when `/config/wifi.txt` exists on microSD.
  - Credentials are not stored in source code or Preferences/NVS.
  - Pi Monitor uses MQTT after Wi-Fi is connected.
  - Credential source: microSD `/config/wifi.txt`.
- **MQTT**
  - Used by Pi Monitor as a Raspberry Pi/device monitor and scoped command publisher.
  - Broker settings come from microSD `/config/pi.txt`.
  - Subscribes to `home/#` for read-only monitoring.
  - Uses `home/devices/<device>/<kind>` topics to update the compact device list.
  - Publishes Cardputer availability/status under `home/devices/scoober-cardputer/...`.
  - Publishes only whitelisted `read_now` and fixed-choice `set_interval` JSON commands to `home/devices/<selected_target>/commands`.
- **I2C**
  - Grove external I2C is used by ENV III on G2/G1.
  - Optional Unit PaHub v2.1 support detects the mux at `0x70` and selects ENV III on channel 0.
  - OLED Status Dashboard and OLED Test select the SSD1309 OLED on PaHub channel 1 and probe `0x3C` / `0x3D`.
  - Internal I2C is used by Cardputer hardware through M5 libraries.
  - Cap LoRa-1262 uses internal I2C G8/G9 for the PI4IOE5V6408 antenna-switch expander at `0x43`.
- **SPI**
  - Used for microSD access.
  - Cap LoRa-1262 SX1262 diagnostics also use EXT SPI pins `G40/G39/G14` with `G5 NSS`; microSD remains on `G12 CS`.
- **LoRa / GNSS**
  - `LoRa Diag` is receive-only and uses RadioLib to initialize SX1262 and start `startReceive()`.
  - Missing cap/radio reports `LoRa not found` on screen and serial instead of blocking the firmware.
  - GNSS is verified separately through ATGM336H UART at `115200` 8N1 with Cardputer RX `G15 GPS-TX` and Cardputer TX `G13 GPS-RX`.
  - TinyGPSPlus parses GNSS NMEA into fix status, satellites, HDOP, coordinates, UTC time, and checksum counters.
  - No transmit behavior is enabled.
- **ESP-NOW**
  - Not implemented.
  - The earlier RC controller placeholder was removed because the user decided this Cardputer project is not the RC controller.
- **BLE**
  - Not implemented.
- **IR**
  - Library dependency exists, but no active feature currently uses IR.
- **Audio**
  - Built-in mic/speaker are used for Voice Memos.

Future communication direction:

- Raspberry Pi command center implementation has started.
- First transport decision: Wi-Fi MQTT.
- First firmware milestone: read-only `Pi Monitor`, confirmed working on hardware on 2026-07-29.
- First command milestone: whitelisted `read_now` publisher to an explicit `command_target`, confirmed working on hardware on 2026-07-30.
- Pi/MQTT settings should come from microSD `/config/pi.txt`.

## Raspberry Pi Command Center Plan

Priority #6 starts with a small read-only MQTT monitor and adds command actions only as whitelisted JSON publishes, never shell control or admin actions.

First milestone:

- Add a `Pi Monitor` menu item. Done in first pass.
- Require Wi-Fi to already be connected through WiFi Connect.
- Read Raspberry Pi MQTT settings from `/config/pi.txt` on microSD.
- Connect to the Raspberry Pi MQTT broker.
- Subscribe to `home/#` for read-only visibility while testing.
- Show MQTT connection status, message count, last topic/payload, and a compact device list on the Cardputer display.
- Keep Backspace return-to-menu behavior.
- Fail gracefully when Wi-Fi, SD config, or the broker is unavailable.

Whitelisted command actions:

- Add `command_target` to `/config/pi.txt`.
- Press `C` on Pi Monitor to publish `{"command":"read_now"}`.
- Press `T` to cycle through the configured target and discovered devices.
- Press `I` to cycle fixed interval choices: 10, 30, 60, and 300 seconds.
- Press `S` to publish `{"command":"set_interval","seconds":<selected>}`.
- Publish to `home/devices/<command_target>/commands`.
- Keep command publishing scoped to these explicit whitelisted commands.
- `read_now` is done and confirmed working on Cardputer hardware on 2026-07-30.
- `set_interval` was added and confirmed working on Cardputer hardware on 2026-08-15.
- Target selection was added and confirmed working on Cardputer hardware on 2026-08-15.

Cardputer MQTT identity:

- Publish retained `online` / `offline` availability to `home/devices/scoober-cardputer/availability`.
- Publish retained status JSON to `home/devices/scoober-cardputer/status`.
- Status includes device id, firmware version, uptime, Wi-Fi RSSI, free heap, and available battery/power hints.
- Refresh status about once per minute while Pi Monitor is open.
- Done and confirmed working on Cardputer hardware on 2026-08-15.

Response display:

- Shows response topics such as `home/devices/<device>/responses` on a dedicated `Resp:` line.
- Also treats fresh status/telemetry from `command_target` after `read_now` as command feedback, because some small device nodes answer by publishing new telemetry instead of a separate response topic.
- Summarizes common JSON response fields such as `ok`, `success`, `status`, `command`, `message`, and `error`.
- Keeps the generic `Last` / `Pay` lines visible for raw topic troubleshooting.

Suggested `/config/pi.txt` format:

```text
mqtt_host=10.0.0.180
mqtt_port=1883
device_id=scoober-cardputer
command_target=esp32-c3-test
```

Reason for MQTT:

- The Raspberry Pi learning repo already uses an MQTT broker/listener and `home/devices/<device>/...` topics.
- The Pi listener already subscribes to `home/#`.
- MQTT can support later safe command actions using existing device `commands` topics.
- HTTP can be revisited later if the Pi dashboard grows a small JSON API.

Rules:

- Reuse the existing WiFi Connect flow; do not add another Wi-Fi credential path.
- Do not hardcode Pi IPs, MQTT settings, Wi-Fi credentials, or MQTT passwords in source.
- Do not commit real `pi.txt` files. This repo ignores `/config/pi.txt` and `/pi.txt` in case local copies are created while testing.
- Do not implement direct shell control, remote command execution, or Raspberry Pi admin actions.
- Do not add arbitrary command text entry; add command actions one whitelisted JSON payload at a time.

## Power Architecture And Voltage Details

Power sources:

- Built-in battery
- USB-C / VBUS

Firmware power behavior:

- Uses `M5.Power.getBatteryVoltage()`.
- Uses `M5.Power.getBatteryLevel()`.
- Uses `M5.Power.getVBUSVoltage()`.
- Uses `M5.Power.getBatteryCurrent()`.
- Uses `M5.Power.isCharging()`.

Important power decision:

- The raw charging/current API was not reliable enough by itself on this hardware.
- The Battery feature estimates Charging vs Not Charging from voltage trend plus VBUS presence.
- Trend sampling must continue even while on the main menu or other feature screens. This fixed the issue where plugging/unplugging outside the Battery screen gave stale or misleading trend values when entering Battery later.
- Battery percentage is shown only when the raw power API is not contradicting the filtered charging trend.
- Raw power API status is still shown on the `API:` line for diagnosis, but it no longer overrides the filtered `Charge:` line.

Voltage behavior observed by user:

- Not charging can fluctuate around +/-10 mV.
- Charging can trend around +100 mV.
- Therefore the firmware uses a +50 mV threshold, VBUS-present gate, and 3-sample confirmation before showing Charging.

## Display Architecture

The firmware originally flickered when dynamic screens redrew the whole display. Current design:

- `drawHeader()` redraws the full header/frame only when entering a screen.
- Dynamic screens call `beginContentDraw()` and `commitContentDraw()` to draw into a `M5Canvas`.
- The content canvas is pushed below the header at `CONTENT_TOP`.
- Battery, System, Environment, and Level screens should not call `drawHeader()` during periodic updates.
- The content canvas is initialized through `initContentCanvas()`, forces non-PSRAM allocation, and falls back from 16-bit to 8-bit color if needed. This avoids feature-screen resets if the sprite buffer cannot be allocated.

Guard script:

- `tools/check_display_refresh.py`

## Important Design Decisions And Reasoning

- Keep this as a stable foundation, not a final monolithic product.
- Prefer simple state-machine screens over deeply nested loop logic.
- Keep all user-facing features reachable from the main menu.
- Use arrow keys and OK/Enter instead of number-key shortcuts.
- Use Backspace as the menu return key.
- Avoid full-screen redraws for changing sensor values to prevent flicker.
- Store saved Wi-Fi names only; no passwords, no automatic connection.
- Use Preferences/NVS for small persistent settings such as saved SSIDs.
- Use microSD for voice memos because audio files are too large for NVS.
- Use Grove I2C for external sensors; do not attach external I2C modules to Cardputer Adv internal G8/G9.
- Keep uncertain hardware APIs defensive. A missing ENV III or SD card should show a status message and leave the firmware usable.
- Keep feature files small enough to understand one screen at a time, with shared declarations centralized in `src/app.h`.

## Known Bugs, Issues, And Limitations

- `pio` may not be on PATH in some shells. Use `python -m platformio`.
- M5Unit-ENV may emit a C++ warning inside its own `I2C_Class.cpp`; the firmware still builds.
- Battery charging/current raw API values may be unknown, zero, or not useful. The trend estimate is the practical source of truth for the Battery screen.
- Wi-Fi scanning is blocking while the scan is running.
- Voice memo playback is currently blocking and not a full background audio player.
- Voice memos depend on the microSD card. If no card is present, the feature shows an error status.
- Voice memo list is capped at 30 displayed WAV files even though names can go up to `memo999.wav`.
- The UI is tuned for the 240x135 built-in screen; long SSIDs and file names are truncated.
- No timezone/clock/date handling exists yet.

## Things Already Tried That Did Not Work

- External SSD1309 OLED on G8/G9:
  - Goal was to keep Grove free.
  - Result: arrow keys stopped working because G8/G9 are the Cardputer Adv internal I2C bus.
  - Decision: remove active OLED feature for now and leave only a future note.
- External SSD1309 OLED on Grove G2/G1:
  - Test worked.
  - User decided not to add an external display right now.
  - Active U8g2 dependency and OLED code were removed.
- Battery status labels like `Watching`, `Likely charging`, or similar:
  - User wanted only `Charging` and `Not charging`.
  - Trend deadzone and confirmation samples were added.
- Resetting battery trend only when entering Battery:
  - Did not work well because plug/unplug events on the main menu produced stale trend behavior.
  - Continuous background sampling fixed it.
- Full display redraws for changing data:
  - Caused flicker on Battery/System/IMU/Keyboard updates.
  - Replaced with canvas content updates.
- Original launcher features:
  - Keyboard Test, Display Test, SD Card Test, and ESP-NOW RC Placeholder were removed.
  - User no longer wants this device to be the RC controller.
- Simple IMU Test:
  - Replaced by the Level feature with crosshair/dot visualization.

## Next Recommended Steps

Highest priority:

1. Hardware-test parsed GNSS status outside or near a window with the Cap LoRa-1262 antenna installed.
2. Confirm `LoRa Diag` moves from `NoFix` to `Fix`, shows satellites/HDOP, and displays latitude/longitude/UTC once the GNSS module has sky view.
3. Keep OLED drawing centralized and keep OLED on PaHub channel 1.
4. Keep ENV III on PaHub channel 0.

Good near-term improvements:

1. Plan Priority #9 RTC module integration after the OLED customization pass.
2. Use RTC time for Environment logs, Voice Memo names, OLED clock/status, and Pi Monitor timestamps where it helps.
3. Revisit `Resp:` only if future commands need explicit success/failure acknowledgments; `Last` / `Pay` are working for now.
4. Add MQTT authentication after live Mosquitto configuration is confirmed.

Future bigger milestones:

1. Priority #10 future LoRa/GNSS features after diagnostics are hardware-tested.
2. Broader Raspberry Pi command center integration.
3. MQTT authentication after live Mosquitto configuration is confirmed.
4. More hardware tools using IR, Grove, BLE, or other Cardputer expansion options.

## Build Instructions

Open this folder in VS Code, then run:

```sh
pio run
```

If `pio` is not available in the shell:

```sh
python -m platformio run
```

## Upload Instructions

Connect the Cardputer Adv with a data-capable USB-C cable, then run:

```sh
pio run --target upload
```

If `pio` is not available:

```sh
python -m platformio run --target upload
```

If upload fails, enter download mode:

1. Turn the top power switch OFF.
2. Hold `G0`.
3. Plug in USB-C or apply power.
4. Release `G0`.
5. Try upload again.

## Serial Monitor

```sh
pio device monitor --baud 115200
```

Or:

```sh
python -m platformio device monitor --baud 115200
```

Serial output includes:

- Boot status
- Firmware name/version
- Keyboard events
- Wi-Fi scan results
- Saved Wi-Fi events
- Voice memo record/play/delete events
- Environment sensor status and readings
- Environment log start/stop events

## Important Commands And Scripts

Build:

```sh
python -m platformio run
```

Build XIAO NRF24 OLED node:

```sh
python -m platformio run -d nodes/xiao_nrf24_oled
```

Upload:

```sh
python -m platformio run --target upload
```

Upload XIAO NRF24 OLED node:

```sh
python -m platformio run -d nodes/xiao_nrf24_oled --target upload
```

Size:

```sh
python -m platformio run --target size
```

Run all guard scripts:

```sh
python tools/check_battery_trend.py
python tools/check_display_refresh.py
python tools/check_environment_feature.py
python tools/check_external_display_revisit.py
python tools/check_level_tool.py
python tools/check_menu_structure.py
python tools/check_nrf24_feature.py
python tools/check_oled_test.py
python tools/check_oled_status_dashboard.py
python tools/check_pi_command_center_plan.py
python tools/check_power_status.py
python tools/check_saved_wifi.py
python tools/check_wifi_credentials_strategy.py
python tools/check_voice_memos.py
python tools/check_wifi_scroll.py
python tools/check_xiao_nrf24_node.py
```

Search code quickly:

```sh
rg "Environment|Voice Memos|Battery|WiFi|LoRa|NRF24" src tools README.md notes.md todo.md
```

## Firmware Test Checklist

After upload:

- Screen boots and shows `M5 Cardputer Lab`.
- Splash screen shows `Scoober`.
- Main menu appears after the splash screen.
- Arrow keys move the highlighted menu item.
- OK/Enter opens the selected feature.
- Backspace returns to the menu from a feature.
- Battery shows voltage, percent, charging estimate, API status, VBUS/current hints, and trend.
- System shows firmware version, uptime, heap, CPU, flash, and screen size.
- WiFi Scan lists nearby networks without credentials and scrolls with arrow keys.
- WiFi Scan opens save confirmation with OK/Enter and rescans with `R`.
- Saved WiFi shows saved SSID names after reboot or power-off.
- Saved WiFi deletes selected names with `D`.
- WiFi Connect handles missing `/config/wifi.txt` gracefully.
- WiFi Connect connects when `/config/wifi.txt` has a valid SSID/password and shows an IP address.
- WiFi Connect times out and shows failure status for wrong credentials.
- WiFi Connect disconnects with `D`.
- User confirmed WiFi Connect testing worked great on Cardputer hardware on 2026-07-23.
- Pi Monitor handles missing `/config/pi.txt` gracefully.
- Pi Monitor shows `Use WiFi Connect` when Wi-Fi is not connected.
- Pi Monitor connects to the configured MQTT broker when `/config/pi.txt` is valid.
- Pi Monitor increments `Msgs` and shows the last topic/payload for incoming `home/#` messages.
- Pi Monitor updates the device list for `home/devices/<device>/<kind>` topics.
- Pi Monitor publishes Cardputer availability to `home/devices/scoober-cardputer/availability` and status JSON to `home/devices/scoober-cardputer/status`.
- User confirmed Cardputer MQTT status/availability publishing works on hardware on 2026-08-15.
- Pi Monitor publishes `read_now` to `home/devices/<command_target>/commands` with `C` when `command_target=esp32-c3-test` is present in `/config/pi.txt`.
- Pi Monitor cycles command targets with `T`.
- User confirmed Pi Monitor target selection works on hardware on 2026-08-15.
- Pi Monitor cycles fixed `set_interval` choices with `I` and publishes the selected interval with `S`.
- Pi Monitor shows response topics such as `home/devices/<device>/responses`, nested response topics, and post-command `command_target` updates on a dedicated `Resp:` line.
- User reported on 2026-08-06 that `Resp:` still stayed on `waiting`; this is not blocking while `Last` / `Pay` and telemetry show command results.
- Pi Monitor disconnects MQTT with `D`, clears/reconnects with `R`, and retries connection with OK/Enter.
- User confirmed Pi Monitor MQTT viewing works on Cardputer hardware on 2026-07-29.
- User confirmed Pi Monitor `read_now` command publishing works on Cardputer hardware on 2026-07-30.
- Voice Memos records WAV files to microSD.
- Voice Memos lists saved memos, plays with OK/Enter, and deletes selected memos with `D`.
- Environment shows live temperature, humidity, and pressure when ENV III is connected.
- Environment shows a not-found/retry message when ENV III is disconnected.
- Environment uses `L to name and start logging`; OK/Enter starts logging and Backspace deletes characters while naming.
- Environment writes `/env/env001.csv` or named files such as `/env/backyard001.csv` to microSD.
- Environment ignores invalid pressure readings instead of logging impossible values.
- Level shows a moving dot and center crosshair using the Cardputer Adv IMU.

## Assumptions And Constraints

- The user wants Codex to edit/generate code and documentation directly.
- Keep the first stable foundation simple and beginner-friendly.
- Do not add RC car control unless the user explicitly changes direction.
- Raspberry Pi networking is now in Priority #6 implementation; the first firmware milestone is a read-only MQTT monitor, followed by whitelisted JSON commands.
- Do not hardcode Wi-Fi credentials.
- Do not store Wi-Fi passwords in Preferences/NVS.
- Wi-Fi credentials should come from microSD `/config/wifi.txt`; do not commit real `wifi.txt` files.
- Raspberry Pi MQTT settings should come from microSD `/config/pi.txt`; do not commit real `pi.txt` files.
- Keep external I2C on Grove unless a hardware expansion plan is explicit.
- Preserve existing working behavior unless the user asks for a change.
- The current codebase is intentionally pragmatic; do not over-refactor unless it helps the current task.

## Troubleshooting

- Use a data-capable USB-C cable, not a charge-only cable.
- Check that PlatformIO selected the correct COM port.
- Use download mode if upload fails.
- Use `python -m platformio` if `pio` is not recognized.
- If the keyboard stops working after adding hardware, immediately suspect an I2C pin conflict.
- If Environment says `ENV III not found`, confirm the unit is in the Grove port or PaHub channel 0 and wait for retry.
- If using Unit PaHub v2.1, leave the DIP switch at default `0x70` for this firmware step.
- If the OLED Status Dashboard stays blank, open OLED Test to see whether the firmware reports `PaHub not found` or `OLED not found`.
- If OLED Test says `PaHub not found`, confirm the PaHub input is connected to Cardputer Grove and the DIP switch is at `0x70`.
- If OLED Test says `OLED not found`, confirm the OLED is on PaHub channel 1 and try OK/Enter or R.
- If Voice Memos shows SD errors, confirm the microSD card is inserted and formatted.
- If needed, restore factory firmware with M5Burner.
