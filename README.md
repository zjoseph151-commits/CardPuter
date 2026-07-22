# Scoober

## Project Overview And Goals

Scoober is custom firmware for the **M5Stack Cardputer Adv Version**. It started as a safe starter firmware called `cardputer-project-launcher` and has evolved into a small, working Cardputer utility hub.

The current goal is not to build the final product in one leap. The goal is to keep a stable firmware foundation that proves custom development on the Cardputer Adv, then add useful features one at a time without breaking the keyboard, display, battery behavior, SD card, or Grove expansion path.

## Quick Start For Next Codex Session

Start here if this repository is opened in a fresh Codex chat.

1. Read this file, then read [notes.md](notes.md) and [todo.md](todo.md).
2. Do not assume unused pins are safe. **Avoid using G8/G9 directly** for external I2C hardware on the Cardputer Adv; those pins share the internal I2C bus and caused keyboard failures during OLED testing.
3. Priority #5 credential strategy is microSD `/config/wifi.txt`, but connection firmware is not implemented yet. Do not hardcode credentials or add `WiFi.begin` except in that intentional milestone.
4. Do not revive ESP-NOW RC controller work. The user decided this device is not going to be the RC controller.
5. RF Scan is the active NRF24 feature. The user confirmed it is working fine on hardware on 2026-07-15.
6. Do not reopen the retired XIAO NRF24 two-node debugging path unless the user explicitly asks.
7. Build with `python -m platformio run` if `pio` is not on PATH.
8. Run all guard scripts before claiming work is complete:

```sh
python tools/check_battery_trend.py
python tools/check_display_refresh.py
python tools/check_environment_feature.py
python tools/check_level_tool.py
python tools/check_menu_structure.py
python tools/check_nrf24_feature.py
python tools/check_oled_test.py
python tools/check_power_status.py
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
- Uses the built-in display as the only UI display.
- Uses the Grove I2C port for the M5Stack ENV III Unit.
- Uses microSD for voice memo storage.
- RF Scan works with the NRF24L01 module and shows quiet channels for future NRF24 projects.
- Has no Wi-Fi credentials and makes no Wi-Fi connection attempts.
- Has an approved future Wi-Fi credential strategy: read `/config/wifi.txt` from microSD, never from source code or NVS.
- Has no active ESP-NOW code.
- Has no active external OLED display code.

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

- M5Stack ENV III Unit
- Sensors used through `M5Unit-ENV`:
  - SHT30 for temperature and humidity
  - QMP6988 for air pressure and altitude estimate

Optional hardware:

- microSD card for Voice Memos
- USB-C data cable for upload and serial monitor
- NRF24L01+ PA+LNA module with adapter/breakout for RF Scan

Hardware intentionally not active right now:

- External SSD1309 OLED display
- ESP-NOW RC controller hardware
- Raspberry Pi networking/control path
- IR, BLE, audio beyond voice memos, and other expansion hardware

Future idea: add SSD1309 OLED support as a secondary display after choosing a safe pin plan or I2C expansion path. Avoid using G8/G9 directly on the Cardputer Adv because those pins share the internal I2C bus with the keyboard.

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
- `nrf24/RF24@^1.6.1`
- `z3t0/IRremote@^4.7.1`

Arduino/core libraries used by the firmware:

- `M5Cardputer.h`
- `M5UnitENV.h`
- `Preferences.h`
- `RF24.h`
- `SPI.h`
- `SD.h`
- `Wire.h`
- `WiFi.h`
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
|   |-- input.cpp
|   |-- level_tool.cpp
|   |-- main.cpp
|   |-- power_screen.cpp
|   |-- rf_scanner.cpp
|   |-- ui.cpp
|   |-- voice_memos.cpp
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
            `-- 2026-07-01-environment-feature.md
```

File responsibilities:

- [src/main.cpp](src/main.cpp): firmware entry point with `setup()` and `loop()`.
- [src/app.h](src/app.h): shared constants, types, global declarations, and function prototypes.
- [src/app_state.cpp](src/app_state.cpp): shared global state, menu definitions, and hardware helper objects.
- [src/ui.cpp](src/ui.cpp): screen routing, header/content drawing helpers, and main menu rendering.
- [src/input.cpp](src/input.cpp): keyboard event dispatch and screen-specific key handling.
- [src/power_screen.cpp](src/power_screen.cpp): Battery/System screens and battery trend logic.
- [src/wifi_screens.cpp](src/wifi_screens.cpp): Wi-Fi scan, saved SSID list, save/delete flows, and Preferences storage.
- [src/voice_memos.cpp](src/voice_memos.cpp): microSD WAV recording, listing, playback, and delete flow.
- [src/environment_screen.cpp](src/environment_screen.cpp): ENV III sensor readings and CSV logging.
- [src/rf_scanner.cpp](src/rf_scanner.cpp): NRF24 radio setup and RF channel scanner.
- [src/level_tool.cpp](src/level_tool.cpp): BMI270 level/crosshair tool.
- [platformio.ini](platformio.ini): board/framework/library configuration.
- [nodes/xiao_nrf24_oled](nodes/xiao_nrf24_oled): separate PlatformIO project for the XIAO ESP32-C3 NRF24/OLED proof node.
- [tools/](tools): lightweight Python guard scripts. These are not full unit tests, but they catch accidental removal of important behavior and design decisions. Firmware guards use [tools/firmware_source.py](tools/firmware_source.py) to scan all `src` files.
- [docs/superpowers/plans/](docs/superpowers/plans): implementation plan history for completed features.
- [notes.md](notes.md): deeper technical notes, lessons learned, and reasoning.
- [todo.md](todo.md): prioritized next work.

## Current Working Features

### Main Menu

Menu items:

1. Battery
2. System
3. WiFi Scan
4. Saved WiFi
5. Voice Memos
6. Environment
7. RF Scan
8. Level

Navigation:

- Arrow keys are represented in the current firmware by keyboard characters:
  - `;` or `,` moves up
  - `.` or `/` moves down
- OK/Enter opens the selected feature.
- Backspace returns to the main menu from most feature screens.
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

### Planned Wi-Fi Credential Strategy

The approved Priority #5 credential strategy is to read Wi-Fi credentials from a microSD card file when a future connect feature is implemented.

Planned file path on the Cardputer microSD card:

```text
/config/wifi.txt
```

Planned file format:

```text
ssid=YourNetworkName
password=YourNetworkPassword
```

Rules for the future connect feature:

- Do not hardcode credentials in source code.
- Do not store Wi-Fi passwords in Preferences/NVS.
- Do not commit real `wifi.txt` files. This repo ignores `/config/wifi.txt` and `/wifi.txt` in case local copies are created while testing.
- Keep the existing Saved WiFi feature as SSID-only unless the user explicitly asks to change it.
- Add `WiFi.begin` only as part of an intentional connection screen or connection helper.
- Connection attempts should show clear status, use a timeout, handle missing SD/config gracefully, and return safely to the menu.

This strategy is documented, but the firmware still does not connect to Wi-Fi yet.

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
- Uses the global Arduino `Wire` object for ENV III because this path reads both SHT30 and QMP6988 correctly on the Cardputer Adv.
- Shows:
  - Temperature in C and F
  - Humidity in percent
  - Pressure in hPa
  - Altitude estimate in meters
  - Sensor status
- Optional CSV logging to microSD.
- Refreshes about once per second.
- If the unit is unplugged or not found, the screen shows `ENV III not found` and retries every few seconds.
- Supports partial sensor availability. If only SHT30 or only QMP6988 responds, the screen can still show the readings it can get.
- Press `L` to name and start logging; press `L` again while logging to stop.
- The log-name screen accepts letters, numbers, spaces, `_`, and `-`.
- Backspace deletes characters while naming; OK/Enter starts logging.
- Blank names fall back to `/env/env001.csv`.
- Named sessions create files such as `/env/backyard001.csv`; spaces become underscores and names are capped at 16 characters.
- CSV columns: `uptime_s,temp_c,temp_f,humidity_pct,pressure_hpa,altitude_m`.
- The Environment screen shows the log file name and sample count while logging.
- If the SD card is missing, logging shows an error and the firmware remains usable.

ENV constants:

- SDA: `ENV_I2C_SDA_PIN = 2`
- SCL: `ENV_I2C_SCL_PIN = 1`
- I2C frequency: `ENV_I2C_FREQUENCY = 400000`
- Refresh: `ENV_REFRESH_INTERVAL_MS = 1000`
- Retry: `ENV_RETRY_INTERVAL_MS = 3000`

ENV wiring:

- Easiest: plug ENV III into the Cardputer Grove port.
- If wiring manually:
  - VCC -> Grove 5V
  - GND -> Grove GND
  - SDA -> Grove SDA
  - SCL -> Grove SCL

### RF Scan

Behavior:

- Adds an RF channel scanner using the Arduino `RF24` library and the NRF24L01 module.
- Uses the Cardputer-Adv EXT header SPI mapping.
- Initializes the radio when the screen opens.
- Shows whether the radio was detected on SPI.
- Sweeps channels `0-125`.
- Takes `RF_SCAN_SAMPLE_COUNT` samples per channel using `testRPD()` / `testCarrier()`.
- Shows a bar graph of activity per channel.
- Highlights the quietest channels in green and lists them as `Quiet`.
- Shows the selected channel and activity score as `Act:n/5`.
- Press `R` or OK/Enter to rescan.
- Press `,` / `;` and `.` / `/` to move the selected channel marker.
- If the radio is missing or wired incorrectly, the screen shows a status message and the firmware remains usable.

RF Scan constants:

- CE: `G4`
- CSN: `G5`
- SCK: `G40`
- MOSI: `G14`
- MISO: `G39`
- SPI frequency: `NRF24_SPI_FREQUENCY = 4000000`
- Channel: `NRF24_CHANNEL = 76`
- Payload size: `NRF24_PAYLOAD_SIZE = 32`
- Data rate: `RF24_250KBPS`
- PA level: `RF24_PA_LOW`
- Scan channels: `RF_SCAN_CHANNEL_COUNT = 126`
- Samples per channel: `RF_SCAN_SAMPLE_COUNT = 5`
- Quiet channel count: `RF_SCAN_QUIET_COUNT = 5`
- Dwell time per sample: `RF_SCAN_DWELL_MS = 2`

NRF24 wiring:

- Wire the NRF24L01 module through its adapter/breakout according to the adapter's power requirements.
- Connect radio logic to the Cardputer-Adv EXT header:
  - CE -> `G4`
  - CSN -> `G5`
  - SCK -> `G40`
  - MOSI -> `G14`
  - MISO -> `G39`
  - GND -> GND
- The EXT SPI pins are shared with the microSD bus. The firmware holds microSD CS `G12` high before initializing NRF24 and uses NRF24 CSN `G5` as the radio chip-select.
- Keep the ENV III on Grove `G2/G1`; do not move NRF24 onto the Grove I2C pins.

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
| NRF24 SCK | G40 | Shared EXT SPI line |
| NRF24 MISO | G39 | Shared EXT SPI line |
| NRF24 MOSI | G14 | Shared EXT SPI line |
| NRF24 CSN | G5 | Dedicated NRF24 chip-select |
| NRF24 CE | G4 | Dedicated NRF24 chip-enable |
| ENV III SDA | G2 / Grove SDA | External Grove I2C |
| ENV III SCL | G1 / Grove SCL | External Grove I2C |
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
  - Used only for station-mode scanning.
  - No credentials.
  - No Wi-Fi connection attempts.
  - No HTTP/MQTT/websocket/network command center yet.
  - Approved future credential source: microSD `/config/wifi.txt`.
- **I2C**
  - Grove external I2C is used by ENV III on G2/G1.
  - Internal I2C is used by Cardputer hardware through M5 libraries.
- **SPI**
  - Used for microSD access.
  - Used by the RF Scan feature on the Cardputer-Adv EXT SPI pins.
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

- Raspberry Pi command center ideas are still future work.
- If Wi-Fi networking is added later, keep secrets out of source code. Use the approved microSD `/config/wifi.txt` strategy first unless the user changes direction.

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

1. Smoke-test the split firmware on Cardputer Adv hardware after upload.
2. Keep testing the Environment screen with the ENV III connected and disconnected.
3. Confirm no keyboard regression while ENV III is plugged into Grove.

Good near-term improvements:

1. Add timestamps if a reliable time source is introduced.
2. Add a simple settings screen for units, refresh rate, and maybe Fahrenheit-only display.
3. Add a file browser or memo management improvements for Voice Memos.
4. Implement a Wi-Fi connection screen using the approved microSD `/config/wifi.txt` strategy.

Future bigger milestones:

1. Raspberry Pi command center integration.
2. Wi-Fi networking after a credential strategy is chosen.
3. Future idea: optional external display support only after a safe pin/I2C expansion plan is chosen.
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
- RF Scan / NRF24 init and detection events

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
python tools/check_level_tool.py
python tools/check_menu_structure.py
python tools/check_nrf24_feature.py
python tools/check_oled_test.py
python tools/check_power_status.py
python tools/check_saved_wifi.py
python tools/check_wifi_credentials_strategy.py
python tools/check_voice_memos.py
python tools/check_wifi_scroll.py
python tools/check_xiao_nrf24_node.py
```

Search code quickly:

```sh
rg "Environment|Voice Memos|Battery|WiFi|RF Scan|NRF24" src tools README.md notes.md todo.md
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
- Wi-Fi still makes no connection attempt until the future SD-backed connect screen is implemented.
- Voice Memos records WAV files to microSD.
- Voice Memos lists saved memos, plays with OK/Enter, and deletes selected memos with `D`.
- Environment shows live temperature, humidity, and pressure when ENV III is connected.
- Environment shows a not-found/retry message when ENV III is disconnected.
- Environment uses `L to name and start logging`; OK/Enter starts logging and Backspace deletes characters while naming.
- Environment writes `/env/env001.csv` or named files such as `/env/backyard001.csv` to microSD.
- RF Scan opens, shows radio detected/not found, sweeps channels `0-125`, and draws a bar graph.
- RF Scan lists quiet channels and lets `R` or OK/Enter rescan.
- RF Scan lets `,` / `;` and `.` / `/` move the selected channel marker.
- Level shows a moving dot and center crosshair using the Cardputer Adv IMU.

## Assumptions And Constraints

- The user wants Codex to edit/generate code and documentation directly.
- Keep the first stable foundation simple and beginner-friendly.
- Do not add RC car control unless the user explicitly changes direction.
- Do not add Raspberry Pi networking until the user asks for that milestone.
- Do not hardcode Wi-Fi credentials.
- Do not store Wi-Fi passwords in Preferences/NVS.
- Future Wi-Fi credentials should come from microSD `/config/wifi.txt`; do not commit real `wifi.txt` files.
- Keep external I2C on Grove unless a hardware expansion plan is explicit.
- Preserve existing working behavior unless the user asks for a change.
- The current codebase is intentionally pragmatic; do not over-refactor unless it helps the current task.

## Troubleshooting

- Use a data-capable USB-C cable, not a charge-only cable.
- Check that PlatformIO selected the correct COM port.
- Use download mode if upload fails.
- Use `python -m platformio` if `pio` is not recognized.
- If the keyboard stops working after adding hardware, immediately suspect an I2C pin conflict.
- If Environment says `ENV III not found`, confirm the unit is in the Grove port and wait for retry.
- If Voice Memos shows SD errors, confirm the microSD card is inserted and formatted.
- If needed, restore factory firmware with M5Burner.
