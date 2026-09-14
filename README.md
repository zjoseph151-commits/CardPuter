# Scoober

## Project Overview And Goals

Scoober is custom firmware for the **M5Stack Cardputer Adv Version**. It started as a safe starter firmware called `cardputer-project-launcher` and has evolved into a small, working Cardputer utility hub.

The current goal is not to build the final product in one leap. The goal is to keep a stable firmware foundation that proves custom development on the Cardputer Adv, then add useful features one at a time without breaking the keyboard, display, battery behavior, SD card, or Grove expansion path.

## Quick Start For Next Codex Session

Start here if this repository is opened in a fresh Codex chat.

1. Read this file, then read [notes.md](notes.md) and [todo.md](todo.md).
2. Do not assume unused pins are safe. **Avoid using G8/G9 directly** for external I2C hardware on the Cardputer Adv; those pins share the internal I2C bus and caused keyboard failures during OLED testing.
3. Priority #5 now includes a hardware-tested WiFi Connect screen that reads credentials from microSD `/config/wifi.txt`. Do not hardcode credentials, and keep `WiFi.begin` limited to that intentional flow.
4. Priority #6 now has a hardware-tested MQTT `Pi Monitor` screen using Raspberry Pi settings from microSD `/config/pi.txt`, hardware-tested Cardputer status/availability publishing, hardware-tested `read_now` and `set_interval` commands, hardware-tested target selection, a project/command UI, and manual `S` paging for MQTT text on the OLED.
5. Priority #12 now has a hardware-tested `SD Manager` for browsing `/config`, `/env`, `/memos`, and `/tracks`, viewing text/CSV files, editing `/config/wifi.txt` plus `/config/pi.txt`, creating config templates, and showing SD card info.
6. Priority #8 now has a hardware-checked command/help OLED pass for `WiFi Connect`, first-entry `Pi Monitor`, `SD Manager`, `RTC`, `GNSS Dash`, `GNSS Sky`, `Return Home`, `Breadcrumbs`, `LoRa Packets`, and `LoRa Diag`. Menu, Battery, System, WiFi Scan, Saved WiFi, Voice Memos, Environment, OLED Test, and Level are intentionally unchanged, and Priority #8 is stopped for now.
7. Priority #11 is active again for LoRa TX planning. The planning doc is [docs/superpowers/plans/2026-09-13-lora-tx-planning.md](docs/superpowers/plans/2026-09-13-lora-tx-planning.md). There is still no active Cardputer transmit firmware; keep `Breadcrumbs`, `LoRa Packets`, and `LoRa Diag` no-transmit/RX-only. The separate XIAO ESP32-S3 Wio-SX1262 LoRa ACK Node scaffold now lives in [nodes/xiao_sx1262_lora_ack](nodes/xiao_sx1262_lora_ack).
8. Priority #7 is using the M5Stack Unit PaHub v2.1 I2C expansion path. ENV III remains on PaHub channel 0, and the SSD1309 OLED remains on PaHub channel 1 as a small OLED Status Dashboard plus diagnostics screen. The later OLED `not found` issue was traced to a bad Grove cable and resolved on 2026-09-05.
9. Do not revive ESP-NOW RC controller work. The user decided this device is not going to be the RC controller.
10. NRF24L01 feature was removed from the active Cardputer firmware on 2026-08-25 so the EXT path is available for Cap LoRa-1262 planning.
11. Priority #9 now has an `RTC` status screen for the DS3231 / AT24C32 module on PaHub channel 5. The missing RTC path is graceful, and the AT24C32 EEPROM is detected but unused.
12. Priority #10 now has an RX-only `LoRa Diag` screen for the M5Stack Cap LoRa-1262. No transmit path is enabled.
13. The first planned TX-capable feature is `LoRa Ping / Range Test`: US 902-928 MHz planning, included antenna installed, 915.0 MHz / 125 kHz / SF12 / CR 4/5 / sync word 0x34 / preamble 20, 2 dBm initial TX power, explicit arm state, canned ASCII payload, rate limit, and the XIAO ESP32-S3 Wio-SX1262 LoRa ACK Node for second-node ACK testing.
14. Do not reopen the retired XIAO NRF24 two-node debugging path unless the user explicitly asks; it is historical only.
15. Build with `python -m platformio run` if `pio` is not on PATH.
16. Run all guard scripts before claiming work is complete:

```sh
python tools/check_battery_trend.py
python tools/check_breadcrumb_logger.py
python tools/check_display_refresh.py
python tools/check_environment_feature.py
python tools/check_external_display_revisit.py
python tools/check_level_tool.py
python tools/check_lora_cap_diag.py
python tools/check_lora_gnss_dashboard.py
python tools/check_lora_gnss_sky_view.py
python tools/check_lora_packet_monitor.py
python tools/check_lora_tx_planning.py
python tools/check_menu_structure.py
python tools/check_nrf24_feature.py
python tools/check_oled_test.py
python tools/check_oled_status_dashboard.py
python tools/check_power_status.py
python tools/check_pi_command_center_plan.py
python tools/check_return_home.py
python tools/check_rtc_status.py
python tools/check_sd_manager.py
python tools/check_saved_wifi.py
python tools/check_wifi_credentials_strategy.py
python tools/check_voice_memos.py
python tools/check_wifi_scroll.py
python tools/check_xiao_nrf24_node.py
python tools/check_xiao_sx1262_node.py
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
- Has a Priority #9 `RTC` status screen for the DS3231 / AT24C32 module on PaHub channel 5, with graceful missing RTC handling.
- Has no active NRF24L01/RF Scan feature or RF24 dependency in the main Cardputer firmware.
- Has an RX-only Cap LoRa-1262 diagnostics screen with graceful `LoRa not found` status and separate GNSS UART byte/line counters.
- Has a GNSS parser for Cap LoRa-1262 using TinyGPSPlus.
- Has a `GNSS Dash` Priority #11 screen that reuses the Cap LoRa-1262 GNSS parser and shows fix state, satellites, HDOP, latitude/longitude, speed, altitude, and UTC.
- Has a `GNSS Sky` Priority #11 screen that parses GSV satellite-in-view data, draws a sky plot using elevation, azimuth, and SNR, and lets the user inspect the selected satellite on the OLED.
- Has a `Return Home` Priority #11 screen that saves one home waypoint and shows distance/bearing back to it from a fresh GNSS fix.
- Has a `Breadcrumbs` Priority #11 screen that writes fresh-fix CSV track points to `/tracks/trackNNN.csv` on microSD without starting the LoRa radio.
- Has a `LoRa Packets` Priority #11 screen that acts as an RX-only packet viewer with packet count, payload preview, RSSI, SNR, frequency, spreading factor, and bandwidth.
- User confirmed Cap LoRa-1262 diagnostics are working on hardware on 2026-08-25.
- User confirmed the Cap LoRa-1262 GNSS parser is working on hardware on 2026-08-26.
- User reported the `GNSS Dash` Priority #11 screen is looking good on hardware on 2026-08-26.
- User reported the base `GNSS Sky` Priority #11 screen is working great on hardware on 2026-08-28.
- User confirmed the `GNSS Sky` selected-satellite update is working on hardware on 2026-08-28.
- User confirmed the `Return Home` Priority #11 screen is working great on hardware on 2026-08-29.
- User reported the `Breadcrumbs` Priority #11 screen is working fine on hardware on 2026-09-09.
- User reported the `LoRa Packets` Priority #11 screen shows idle channel/noise RSSI on hardware; matching-frame decode belongs to the upcoming second-node TX/range test.
- User confirmed the clarified idle/listening `LoRa Packets` behavior is working fine on hardware on 2026-09-13.
- LoRa TX planning started on 2026-09-13 in [docs/superpowers/plans/2026-09-13-lora-tx-planning.md](docs/superpowers/plans/2026-09-13-lora-tx-planning.md); no active Cardputer transmit firmware has been added yet.
- Has a separate XIAO ESP32-S3 Wio-SX1262 LoRa ACK Node scaffold in [nodes/xiao_sx1262_lora_ack](nodes/xiao_sx1262_lora_ack), using `SCBR,NODE,1,xiao-sx1262-ack` manual probes, `SCBR,ACK,1,xiao-sx1262-ack` ping replies, no periodic beacons, B2B defaults for `GPIO41` NSS / `GPIO39` DIO1 / `GPIO38` RF switch, and the same 915.0 MHz / 125 kHz / SF12 / CR 4/5 / sync word 0x34 / 2 dBm settings as the RX screens.
- Has a hardware-tested WiFi Connect screen that reads `/config/wifi.txt` from microSD and never stores Wi-Fi passwords in source code or NVS.
- Has a hardware-tested Pi Monitor screen that reads `/config/pi.txt`, connects to MQTT, subscribes to Raspberry Pi home IoT device topics, publishes Cardputer status/availability, and publishes whitelisted MQTT commands.
- User confirmed the Pi Monitor project/command UI and manual `S` OLED MQTT message paging are working on hardware on 2026-09-05.
- Has a hardware-tested `SD Manager` that can browse `/config`, `/env`, `/memos`, and `/tracks`, view small text files and large CSV previews, edit known key/value config files, create config templates, show SD card info, and create, rename, or delete files only after confirmation.
- User confirmed the Priority #12 hardware checklist and polish spot-checks passed on 2026-09-08.
- Has no active ESP-NOW code.
- Has an OLED Status Dashboard for the SSD1309 OLED on PaHub channel 1.
- Priority #8 has compact 5x7 OLED command/help sheets for WiFi Connect, Pi Monitor project-list entry, SD Manager, RTC, GNSS Dash, GNSS Sky, Return Home, Breadcrumbs, LoRa Packets, and LoRa Diag.
- User confirmed the Priority #8 command/help OLED pass is working on hardware on 2026-09-13, and Priority #8 is stopped for now.
- Keeps OLED Test as a diagnostics/proof-of-life screen.
- User confirmed OLED Test works on hardware on 2026-08-15.
- User confirmed the OLED Status Dashboard is working across the tested features on 2026-08-22.
- User confirmed a later OLED `not found` issue was fixed by replacing the Grove cable on 2026-09-05.

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

- microSD card for Voice Memos, Environment logs, Wi-Fi/Pi config, and SD Manager browsing/editing
- USB-C data cable for upload and serial monitor
- SSD1309 OLED on PaHub channel 1 for the OLED Status Dashboard and OLED Test diagnostics screen
- DS3231 / AT24C32 I2C RTC module on PaHub channel 5 for the Priority #9 `RTC` status screen
- M5Stack Cap LoRa-1262 for Cardputer Adv, diagnostics plus GNSS feature screens
- Seeed Studio XIAO ESP32-S3 plus Wio-SX1262 for the separate second-node ACK/probe sketch

Hardware intentionally not active right now:

- ESP-NOW RC controller hardware
- Direct Raspberry Pi shell/admin control; Pi Monitor stays scoped to MQTT monitoring plus whitelisted JSON commands
- NRF24L01 / RF Scan firmware feature; removed to avoid conflicts with the upcoming Cap LoRa-1262 EXT interface path
- RTC NTP/build-time setting is active; timestamp consumers are not active yet, and the AT24C32 EEPROM unused boundary remains in place
- Cap LoRa-1262 transmit features; RX diagnostics and LoRa TX planning are active, but no active Cardputer transmit firmware is added until antenna, legal region/frequency, bandwidth/spreading plan, and TX power are deliberately set
- IR, BLE, audio beyond voice memos, and other expansion hardware

Current external-display direction: keep the built-in LCD as the primary control UI, and use the SSD1309 OLED as a small glance/status display on the M5Stack Unit PaHub v2.1. ENV III remains on PaHub channel 0, SSD1309 OLED remains on PaHub channel 1, and DS3231 / AT24C32 RTC remains on PaHub channel 5. PaHub default address is `0x70`. Priority #8 now has a first command/help OLED pass for the remaining feature screens that need on-device command reminders. Avoid using G8/G9 directly on the Cardputer Adv because those pins share the internal I2C bus with the keyboard.

Next planned hardware:

- Priority #9: DS3231 / AT24C32 I2C RTC module.
  - First milestone is active as `RTC`: read DS3231 date/time/temperature on PaHub channel 5, probe AT24C32 at `0x57`, and keep missing RTC behavior graceful.
  - Likely future use: timestamps for logs, voice memo file names, OLED clock/status, and Pi Monitor events.
- Priority #10: M5Stack Cap LoRa-1262 for Cardputer Adv.
  - Hardware includes SX1262 LoRa and ATGM336H GNSS.
  - It connects through the Cardputer Adv EXT interface.
  - The old NRF24/RF Scan firmware path has been removed so LoRa/GNSS diagnostics can claim the EXT path cleanly.
  - First milestone is active as `LoRa Diag`: detect the PI4IOE5V6408 antenna-switch expander, initialize SX1262 with RadioLib in receive-only mode, and count GNSS UART bytes/NMEA lines.
  - GNSS parser milestone uses TinyGPSPlus to show fix status, satellites, HDOP, coordinates, UTC time, and checksum counters.
  - Priority #11 started with `GNSS Dash`, a separate GNSS dashboard that does not start the LoRa radio or transmit.
  - `GNSS Sky` adds a no-transmit satellite sky plot from GSV elevation/azimuth/SNR data.
  - `Return Home` adds a no-transmit saved home waypoint with GNSS distance and bearing.
  - `Breadcrumbs` adds no-transmit GNSS CSV track logging to `/tracks/trackNNN.csv` on microSD.
  - `LoRa Packets` adds an RX-only packet viewer for matching LoRa settings.
- Priority #11: Cap LoRa-1262 transmit-capable feature planning.
  - Planning doc: [docs/superpowers/plans/2026-09-13-lora-tx-planning.md](docs/superpowers/plans/2026-09-13-lora-tx-planning.md).
  - Current assumption: US 902-928 MHz, matching the current 915.0 MHz RX monitors.
  - First TX feature: `LoRa Ping / Range Test`, with included antenna installed, explicit arm state, canned ASCII payload, 2 dBm initial TX power, packet rate limit, and the XIAO ESP32-S3 Wio-SX1262 LoRa ACK Node for ACK testing.
  - There is no active Cardputer transmit firmware yet; keep `LoRa Diag` and `LoRa Packets` RX-only until the dedicated TX feature is deliberately added.

## Software, Libraries, And Frameworks

Development environment:

- PlatformIO
- Arduino framework
- VS Code / PlatformIO extension recommended

PlatformIO environment:

- `default_envs = m5stack-stamps3`
- `platform = https://github.com/pioarduino/platform-espressif32.git#54.03.21`
- `board = m5stack-stamps3`
- `framework = arduino`
- `monitor_speed = 115200`
- `upload_speed = 1500000`
- Arduino-ESP32: `3.2.1`
- ESP-IDF: `5.4.x`
- Tool override: `platformio/tool-esptoolpy@1.40801.0` for Python 3.13 compatibility
- Esptool compatibility script: `pre:tools/esptool_compat.py` and `post:tools/esptool_compat.py`

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
|   |-- breadcrumb_logger.cpp
|   |-- environment_screen.cpp
|   |-- gnss_dashboard.cpp
|   |-- gnss_sky_view.cpp
|   |-- i2c_hub.cpp
|   |-- input.cpp
|   |-- level_tool.cpp
|   |-- lora_diag.cpp
|   |-- lora_gnss.cpp
|   |-- lora_packet_monitor.cpp
|   |-- main.cpp
|   |-- oled_test.cpp
|   |-- pi_monitor.cpp
|   |-- power_screen.cpp
|   |-- return_home.cpp
|   |-- rtc_status.cpp
|   |-- sd_manager.cpp
|   |-- shared_spi.cpp
|   |-- ui.cpp
|   |-- voice_memos.cpp
|   |-- wifi_connect.cpp
|   `-- wifi_screens.cpp
|-- nodes/
|   |-- xiao_nrf24_oled/
|   |   |-- README.md
|   |   |-- platformio.ini
|   |   `-- src/
|   |       `-- main.cpp
|   `-- xiao_sx1262_lora_ack/
|       |-- README.md
|       |-- platformio.ini
|       `-- src/
|           `-- main.cpp
|-- tools/
|   |-- check_battery_trend.py
|   |-- check_breadcrumb_logger.py
|   |-- check_display_refresh.py
|   |-- check_environment_feature.py
|   |-- check_external_display_revisit.py
|   |-- check_level_tool.py
|   |-- check_lora_cap_diag.py
|   |-- check_lora_gnss_dashboard.py
|   |-- check_lora_gnss_sky_view.py
|   |-- check_lora_packet_monitor.py
|   |-- check_lora_tx_planning.py
|   |-- check_menu_structure.py
|   |-- check_nrf24_feature.py
|   |-- check_oled_test.py
|   |-- check_oled_status_dashboard.py
|   |-- check_pi_command_center_plan.py
|   |-- check_power_status.py
|   |-- check_return_home.py
|   |-- check_rtc_status.py
|   |-- check_sd_manager.py
|   |-- check_saved_wifi.py
|   |-- check_wifi_credentials_strategy.py
|   |-- check_voice_memos.py
|   |-- check_wifi_scroll.py
|   |-- firmware_source.py
|   |-- check_xiao_nrf24_node.py
|   `-- check_xiao_sx1262_node.py
`-- docs/
    `-- superpowers/
        `-- plans/
            |-- 2026-06-27-saved-wifi-feedback-delete.md
            |-- 2026-06-27-saved-wifi-names.md
            |-- 2026-06-28-voice-memos.md
            |-- 2026-07-01-environment-feature.md
            |-- 2026-07-25-raspberry-pi-command-center.md
            `-- 2026-09-13-lora-tx-planning.md
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
- [src/sd_manager.cpp](src/sd_manager.cpp): Priority #12 SD Manager for known-folder browsing, small text preview, guarded key/value config editing, and confirmed file create/rename/delete actions.
- [src/breadcrumb_logger.cpp](src/breadcrumb_logger.cpp): Priority #11 Breadcrumb Logger using the shared GNSS parser and microSD CSV track logging.
- [src/voice_memos.cpp](src/voice_memos.cpp): microSD WAV recording, listing, playback, and delete flow.
- [src/i2c_hub.cpp](src/i2c_hub.cpp): optional M5Stack Unit PaHub v2.1 detection and channel selection for shared Grove I2C.
- [src/environment_screen.cpp](src/environment_screen.cpp): ENV III sensor readings and CSV logging.
- [src/oled_test.cpp](src/oled_test.cpp): SSD1309 OLED Status Dashboard, Priority #8 compact command/help sheets, and OLED Test diagnostics on PaHub channel 1.
- [src/rtc_status.cpp](src/rtc_status.cpp): Priority #9 DS3231 / AT24C32 RTC status screen on PaHub channel 5.
- [src/lora_gnss.cpp](src/lora_gnss.cpp): shared Cap LoRa-1262 ATGM336H GNSS UART and TinyGPSPlus parser state.
- [src/gnss_dashboard.cpp](src/gnss_dashboard.cpp): Priority #11 GNSS dashboard using the shared parser without starting the LoRa radio.
- [src/gnss_sky_view.cpp](src/gnss_sky_view.cpp): Priority #11 GNSS Satellite Sky View using GSV elevation, azimuth, and SNR data from the shared parser.
- [src/return_home.cpp](src/return_home.cpp): Priority #11 Waypoint / Return Home screen using a saved home point, GNSS distance, and bearing without starting the LoRa radio.
- [src/lora_packet_monitor.cpp](src/lora_packet_monitor.cpp): Priority #11 RX-only LoRa Packet Monitor using RadioLib and the Cap LoRa-1262 SX1262.
- [src/lora_diag.cpp](src/lora_diag.cpp): RX-only M5Stack Cap LoRa-1262 diagnostics using RadioLib for SX1262 plus the shared parsed GNSS status.
- [docs/superpowers/plans/2026-09-13-lora-tx-planning.md](docs/superpowers/plans/2026-09-13-lora-tx-planning.md): Priority #11 LoRa TX planning gate before transmit-capable firmware work.
- [src/shared_spi.cpp](src/shared_spi.cpp): external SPI chip-select and owner handoff helper for LoRa/microSD sharing.
- [src/level_tool.cpp](src/level_tool.cpp): BMI270 level/crosshair tool.
- [platformio.ini](platformio.ini): board/framework/library configuration.
- [nodes/xiao_nrf24_oled](nodes/xiao_nrf24_oled): archived separate PlatformIO project for the XIAO ESP32-C3 NRF24/OLED proof node.
- [nodes/xiao_sx1262_lora_ack](nodes/xiao_sx1262_lora_ack): separate PlatformIO project for the XIAO ESP32-S3 Wio-SX1262 LoRa ACK Node used as the first `LoRa Ping / Range Test` bench partner.
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
7. SD Manager
8. Voice Memos
9. Environment
10. OLED Test
11. RTC
12. GNSS Dash
13. GNSS Sky
14. Return Home
15. Breadcrumbs
16. LoRa Packets
17. LoRa Diag
18. Level

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
- Priority #8 OLED behavior: `WiFi Connect` shows a compact 5x7 banner, `/config/wifi.txt` purpose, retry/disconnect/back commands, SSID hint, and IP/status summary while leaving the built-in LCD unchanged.

### Pi Monitor

Behavior:

- Raspberry Pi MQTT monitor with a project list, command list, Home / Diagnostics selection, and two whitelisted command actions.
- Requires Wi-Fi to already be connected through WiFi Connect.
- Reads Raspberry Pi MQTT settings from a microSD card file.
- Connects to the MQTT broker with `PubSubClient`.
- Subscribes to `home/#` so terminal-published test messages are visible.
- Treats `home/devices/<device>/<kind>` topics as structured device-list updates.
- Publishes retained Cardputer availability to `home/devices/scoober-cardputer/availability`.
- Publishes retained Cardputer status JSON to `home/devices/scoober-cardputer/status` after MQTT connect and about once per minute while Pi Monitor is open.
- Uses an MQTT last-will so unexpected disconnects can mark Cardputer availability as `offline`.
- User confirmed Cardputer MQTT status/availability publishing works on hardware on 2026-08-15.
- Opens on an LCD project list with `Home / Diagnostics`, configured projects, and discovered device IDs.
- Priority #8 OLED behavior: the first-entry project list shows a compact 5x7 feature description and command sheet for arrows, OK, Backspace, `C`, `T`, `I`, `S`, `R`, and `D`.
- In `Home / Diagnostics`, the LCD shows broker/device diagnostics while the OLED shows one header plus the manually paged latest MQTT message from all MQTT traffic.
- Selecting a project opens an LCD command list for that project's command profile.
- In a selected project, the OLED shows one header plus the manually paged latest MQTT message filtered to that project only.
- User confirmed the project/command UI and manual `S` OLED MQTT message paging are working on hardware on 2026-09-05.
- Displays MQTT status, broker address, message count, last topic/payload, command response display, and compact project/device status.
- Displays the most recent command status on the monitor screen.
- Shows `home/devices/<device>/responses`, nested response topics, or post-command updates from `command_target` on a clearer `Resp:` line.
- Current caveat: user hardware testing on 2026-08-06 still showed `Resp: waiting.`, so `Last` / `Pay` remain the reliable command-feedback view for now.
- Parses small JSON status/telemetry payloads with ArduinoJson.
- `C` publishes `{"command":"read_now"}` to `home/devices/<command_target>/commands`.
- `T` cycles the command target through `command_target` and discovered device IDs.
- `I` cycles fixed `set_interval` choices: 10, 30, 60, and 300 seconds.
- `S` advances the OLED MQTT message page.
- Does not publish arbitrary command text, run shell actions, or perform Raspberry Pi admin actions.
- Disconnects MQTT when leaving the screen.
- OK/Enter opens the highlighted project or sends the highlighted command; in `Home / Diagnostics`, OK/Enter retries MQTT connection.
- Backspace returns from a Pi Monitor subview to the project list before returning to the main menu.
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
project=esp32-c3-test|ESP32-C3 Test|basic
```

`command_target` remains supported as the initial/default command target. Add one `project=id|Label|profile` line per MQTT project; the initial firmware command profile is `basic`, which exposes `read_now` and fixed-choice `set_interval`.

Rules for Pi Monitor:

- Reuse WiFi Connect for Wi-Fi; do not add another Wi-Fi credential path.
- Do not hardcode Pi IPs, MQTT settings, Wi-Fi credentials, or MQTT passwords in source.
- Do not commit real `pi.txt` files. This repo ignores `/config/pi.txt` and `/pi.txt` in case local copies are created while testing.
- Keep command publishing whitelisted and explicit. The allowed commands are `read_now` and fixed-choice `set_interval` to `home/devices/<command_target>/commands`.
- Keep project additions data-driven through `/config/pi.txt` `project=id|Label|profile` lines and firmware command-profile definitions.
- Do not add free-form interval entry; keep `set_interval` seconds limited to firmware-defined choices.
- Cardputer may publish only scoped identity/status topics for its own configured `device_id`.
- User confirmed Pi Monitor MQTT viewing works on Cardputer hardware on 2026-07-29.
- User confirmed Pi Monitor `read_now` command publishing works on Cardputer hardware on 2026-07-30.

### SD Manager

Behavior:

- First Priority #12 safe SD Manager milestone.
- Uses the shared SD SPI handoff helper before SD access.
- Lets the user browse `/config`, `/env`, `/memos`, and `/tracks` from a fixed folder list.
- Shows file names, file sizes, and small text files up to `SD_MANAGER_TEXT_MAX_BYTES = 2048`.
- Treats `.wav`, large non-CSV text, and unknown binary files as metadata/view/delete-only entries.
- Lets large CSV files show a capped first-page preview instead of metadata-only.
- Provides a key/value editor for `/config/wifi.txt` and `/config/pi.txt`.
- Recognizes Wi-Fi and Pi config file names case-insensitively on the SD card.
- Keeps Wi-Fi config compatible with `ssid=...` and `password=...`.
- Keeps Pi Monitor config compatible with `mqtt_host`, `mqtt_port`, `device_id`, `command_target`, and repeated `project=id|Label|profile` lines.
- Saves edited configs through a temp-file save followed by rename, with a backup rename fallback around the original file.
- Always flushes and closes files after writes.
- Can create, rename, and delete files only after confirmation; rename preserves the original file extension when the new name has no extension.
- Sniffs extensionless `/env` files so renamed CSV logs can still be viewed as small text files.
- Keeps a newly created or renamed file selected after the folder refresh.
- Adds an `I` SD card info view with mounted state, card type, used/free space, and known-folder count.
- Adds `/config` shortcuts for confirmed `wifi.txt` and `pi.txt` template creation.
- Blocks SD access while Voice Memos is recording, Environment logging is active, or LoRa diagnostics are actively using the shared EXT SPI path.
- Priority #8 OLED behavior: `SD Manager` shows a compact 5x7 banner and command sheet for arrows, OK, Backspace, `R`, `I`, `N`, `D`, `E`, `W`, and `P`.
- User confirmed the Priority #12 hardware checklist and polish spot-checks passed on 2026-09-08.
- Priority #12 polish added on 2026-09-08.

Control summary:

- In the folder list, arrows select `/config`, `/env`, `/memos`, or `/tracks`; OK opens the selected folder, and `I` opens the SD card info view.
- In a folder, arrows select files, OK views the selected file, and `N` starts a confirmed file create flow.
- In `/config`, `W` and `P` prefill confirmed `wifi.txt` and `pi.txt` template creation.
- In file view, `E` edits known config files, `R` starts a confirmed rename flow, and `D` starts a confirmed delete flow.
- In the key/value editor, arrows select an editable key, OK edits the selected value, typed characters append to the value, Backspace deletes characters, and OK saves with temp-file rename.
- Backspace steps back through SD Manager views before returning to the main menu.

Guard:

- `tools/check_sd_manager.py`

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
- M5Unified Mic.record queues capture asynchronously, so recording waits for each queued chunk to finish before writing the filled buffer to the WAV file.
- The recording screen shows a live input peak value plus mic status on the LCD and OLED. If peak stays near the observed floor of `8` while speaking, the mic path is suspect; if peak moves, the saved WAV should contain audio samples.
- Voice Memos probes right/left/stereo input channels before recording and keeps the loudest working channel for the session. The status line shows values like `Probe R ADV`, `Mic R pk 512`, or `Mic L quiet 8`.
- Cardputer Adv uses the ES8311 codec path before recording: data `G46`, LRCK/WS `G43`, BCLK `G41`, ES8311 address `0x18`. Non-Adv Cardputer uses the built-in PDM path on data `G46` / WS `G43` with no BCLK.
- The Cardputer Adv silent-mic symptom where every channel stays at `peak:8` matches a known ESP-IDF 5.5 I2S regression. The main PlatformIO environment is pinned to pioarduino `54.03.21`, which uses Arduino-ESP32 `3.2.1` and the ESP-IDF 5.4.x family.

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
- RTC PaHub channel: `I2C_HUB_RTC_CHANNEL = 5`
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
- Plug the DS3231 / AT24C32 RTC module into PaHub channel 5.
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
- Starts OLED detection at 400 kHz, then retries at 100 kHz if the display does not ACK.
- Keeps OLED bus-speed changes temporary so ENV III and RTC continue using the normal shared PaHub bus path.
- The built-in Cardputer LCD remains the main control screen.
- The OLED is a small glance/status display, not a duplicate of the built-in screen.
- Dashboard drawing is centralized in `renderOledStatusDashboard()`.
- `serviceOledStatusDashboard()` refreshes the OLED about once per second from `loop()`.
- Features can later provide a short context line through `setOledStatusLine(...)` without knowing U8g2 details.
- Priority #8 adds compact 5x7 command/help OLED sheets for feature screens that benefit from on-device command reminders.
- Missing PaHub or missing OLED shows a status on the built-in OLED Test screen and keeps firmware usable.
- OLED detection retries periodically when the dashboard is active.
- Do not connect the OLED to G8/G9.

Dashboard content:

- Current screen/mode.
- Battery level or charging status.
- Wi-Fi connected/disconnected.
- MQTT connected/disconnected when Pi Monitor is active or has been used.
- A compact context line for the active feature.
- Compact 5x7 command/help sheets for WiFi Connect, Pi Monitor first-entry/project list, SD Manager, RTC, GNSS Dash, GNSS Sky, Return Home, Breadcrumbs, LoRa Packets, and LoRa Diag.

Screen-specific examples:

- Main menu: `Scoober`, battery, Wi-Fi, mode/menu selection.
- WiFi Connect: banner, `/config/wifi.txt` purpose, retry, disconnect, and back commands.
- Pi Monitor: first-entry project list shows feature purpose plus arrows, OK, Back, `C`, `T`, `I`, `S`, `R`, and `D` command reminders; project command and diagnostics subviews keep the existing manually paged MQTT message text using the smaller 5x7 font.
- SD Manager: banner plus arrows, OK, Back, refresh/rename, info, create, delete, edit, and Wi-Fi/Pi template commands.
- Environment: temperature F, humidity, pressure value or `Press: invalid`.
- Voice Memos: `REC mm:ss` while recording, active/selected memo, SD/status.
- RTC: banner plus DS3231 purpose, NTP/build-time set commands, retry, back, and PaHub channel reminders.
- GNSS Dash: banner plus GNSS parser purpose, restart/back commands, and no-LoRa/no-TX reminder.
- GNSS Sky: no banner; starts with satellite-selection/restart/back commands, then selected satellite PRN/SNR/elevation/azimuth/compass/age data.
- Return Home: banner plus saved-home navigation description, save/update, clear, restart, back, distance, and bearing.
- Breadcrumbs: banner plus GNSS CSV track-log description, start/stop, restart, back, point/missed counts, and file hint.
- LoRa Packets: banner plus RX-only packet viewer command reminders, packet/error count summary, matching-sender hint, and no-TX reminder.
- LoRa Diag: banner plus Cap LoRa/GNSS diagnostics command reminders, packet/NMEA summary, and no-TX reminder.
- Level: compact level/tilt context.

OLED Test diagnostics:

- Draws `Scoober OLED`, `SSD1309 ch1`, the active address, a draw counter, a border, and a moving marker.
- Shows the active OLED address, bus speed, and a short PaHub channel 1 probe summary on the built-in LCD.
- OK/Enter or R retries OLED detection.
- Backspace returns to the main menu.

OLED constants:

- PaHub channel: `I2C_HUB_OLED_CHANNEL = 1`
- Primary OLED address: `OLED_I2C_ADDRESS_PRIMARY = 0x3C`
- Secondary OLED address: `OLED_I2C_ADDRESS_SECONDARY = 0x3D`
- OLED fast I2C frequency: `OLED_I2C_FAST_FREQUENCY = ENV_I2C_FREQUENCY`
- OLED fallback I2C frequency: `OLED_I2C_FALLBACK_FREQUENCY = 100000U`
- OLED probe attempts per speed: `OLED_PROBE_ATTEMPTS = 3`
- OLED Test refresh interval: `OLED_TEST_REFRESH_INTERVAL_MS = 1000`
- OLED Status Dashboard refresh interval: `OLED_STATUS_REFRESH_INTERVAL_MS = 1000`
- OLED retry interval: `OLED_RETRY_INTERVAL_MS = 3000`
- OLED status lines: `OLED_STATUS_LINE_COUNT = 5`
- OLED line length: `OLED_STATUS_MAX_CHARS = 21`
- Pi Monitor OLED message lines: `PI_MONITOR_OLED_MESSAGE_LINE_COUNT = 8`
- Pi Monitor OLED message line length: `PI_MONITOR_OLED_MESSAGE_MAX_CHARS = 25`

### RTC Status

Behavior:

- Adds `RTC` as the first Priority #9 firmware milestone.
- Uses the shared Grove I2C bus through the Unit PaHub v2.1.
- Selects the DS3231 / AT24C32 module on PaHub channel 5 before every RTC probe/read.
- Reads DS3231 date, time, oscillator-stopped status, and temperature with raw Arduino `Wire` calls.
- Press `N` to set or correct the DS3231 from NTP local time when Wi-Fi is already connected through WiFi Connect.
- Press `S` only as an offline fallback to set the DS3231 from firmware build date/time plus current Cardputer uptime.
- Probes the AT24C32 EEPROM at `0x57`, but keeps the EEPROM unused until there is a clear reason to store RTC-specific data there.
- Shows `RTC online`, `RTC set NTP`, `RTC needs set`, `NTP sync failed`, `Use WiFi Connect`, `RTC time invalid`, `DS3231 not found`, or `PaHub ch5 missing` without blocking the rest of the firmware.
- Shows a read-attempt counter on the built-in LCD so OK/Enter or R retry actions are visible.
- Missing RTC hardware is graceful: the menu keeps working, the screen shows status, and the feature retries periodically.
- Priority #8 OLED behavior: `RTC` shows a compact 5x7 banner and command sheet for `N` NTP set, `S` build-time set, OK/`R` retry, Backspace, WiFi Connect dependency, and PaHub channel 5.
- Does not initialize SX1262, claim shared SPI, start GNSS serial, or add any LoRa transmit behavior.
- Does not call `WiFi.begin`; RTC uses the existing WiFi Connect flow and only syncs from NTP when Wi-Fi is already connected.
- OK/Enter or R retries RTC detection/read.
- Backspace returns to the main menu.

RTC constants:

- RTC PaHub channel: `I2C_HUB_RTC_CHANNEL = 5`
- DS3231 address: `RTC_DS3231_ADDRESS = 0x68`
- AT24C32 address: `RTC_AT24C32_ADDRESS = 0x57`
- DS3231 control register: `RTC_DS3231_CONTROL_REGISTER = 0x0E`
- Refresh: `RTC_REFRESH_INTERVAL_MS = 1000`
- Retry: `RTC_RETRY_INTERVAL_MS = 3000`
- NTP sync timeout: `RTC_NTP_SYNC_TIMEOUT_MS = 10000`
- NTP poll interval: `RTC_NTP_POLL_MS = 250`
- NTP timezone: `RTC_TIMEZONE_POSIX = "MST7MDT,M3.2.0,M11.1.0"`
- NTP servers: `pool.ntp.org`, `time.nist.gov`

RTC wiring:

- Plug the Unit PaHub v2.1 input into the Cardputer Grove port.
- Leave ENV III on PaHub channel 0.
- Leave the SSD1309 OLED on PaHub channel 1.
- Plug the DS3231 / AT24C32 RTC module into PaHub channel 5.
- Leave the PaHub DIP switch at default address `0x70`.

Guard:

- `tools/check_rtc_status.py`

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
- User confirmed parsed GNSS status is working on hardware on 2026-08-26.
- Priority #8 OLED behavior: `LoRa Diag` shows a compact 5x7 banner and command sheet for OK/`R` restart, Backspace, RX-only/no-TX status, SX1262/GNSS purpose, and packet/NMEA summary.
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
- SD-backed features explicitly drive LoRa `NSS` high and re-begin SPI for microSD after LoRa diagnostics have owned the bus.
- The firmware only services the SX1262 LoRa radio while the `LoRa Diag` or `LoRa Packets` screen is active; the shared GNSS parser is also used by `GNSS Dash`, `GNSS Sky`, `Return Home`, and `Breadcrumbs`.
- Do not use G8/G9 directly for external OLED/ENV hardware; on this cap they are the internal I2C path required by the cap expander.
- No transmit work should be added until the antenna is installed and the legal region/frequency, bandwidth/spreading plan, and TX power are intentionally chosen.

### GNSS Dashboard

Behavior:

- Adds `GNSS Dash` as the first Priority #11 Cap LoRa-1262 feature-expansion screen.
- Reuses the shared ATGM336H GNSS UART/TinyGPSPlus parser in `src/lora_gnss.cpp`.
- Starts only the GNSS serial parser; it does not initialize SX1262, claim the shared SPI bus, or transmit.
- Shows fix/no-fix state, satellites, HDOP, latitude, longitude, speed, altitude, UTC clock, date, fix age, NMEA line count, checksum counts, and byte count.
- Priority #8 OLED behavior: `GNSS Dash` shows a compact 5x7 banner and command sheet for OK/`R` restart, Backspace, GNSS parser purpose, and no-LoRa/no-TX status.
- OK/Enter or R restarts the parser.
- Backspace returns to the main menu and stops the GNSS serial object.
- No transmit path is enabled in this milestone.
- User reported this screen is looking good on Cardputer hardware on 2026-08-26.

Guard:

- `tools/check_lora_gnss_dashboard.py`

### GNSS Satellite Sky View

Behavior:

- Adds `GNSS Sky` as the second Priority #11 Cap LoRa-1262 feature-expansion screen.
- Reuses the shared ATGM336H GNSS UART/TinyGPSPlus parser in `src/lora_gnss.cpp`.
- Parses `$GxGSV` satellite-in-view NMEA sentences for constellation/PRN, elevation, azimuth, and SNR.
- Draws a circular sky plot with horizon/elevation rings, cardinal directions, and SNR-colored satellite dots.
- Tracks GSV sentence count, reported satellites-in-view, active plotted satellites, last GSV age, and strongest SNR satellite.
- Expires satellite positions after `GNSS_SKY_STALE_MS = 15000` so stale GSV data does not look live.
- Arrow keys cycle the selected satellite through active plotted satellites and skip inactive/stale entries.
- The built-in LCD highlights the selected satellite dot while keeping the right-side panel focused on overall sky/fix summary.
- Priority #8 OLED behavior: `GNSS Sky` removes the OLED banner and uses compact 5x7 lines for arrows, OK/`R`, Backspace, and selected satellite label, constellation, PRN, SNR, elevation, azimuth, compass direction, age, and sky count.
- The selection resets or clamps when the active satellite list changes and shows `Sel: none` gracefully when zero satellites are plotted.
- Starts only the GNSS serial parser; it does not initialize SX1262, claim the shared SPI bus, or transmit.
- OK/Enter or R restarts the parser.
- Backspace returns to the main menu and stops the GNSS serial object.
- No transmit path is enabled in this milestone.
- User confirmed the selected-satellite update is working on hardware on 2026-08-28.

Guard:

- `tools/check_lora_gnss_sky_view.py`

### Waypoint / Return Home

Behavior:

- Adds `Return Home` as the next Priority #11 Cap LoRa-1262 feature-expansion screen.
- Uses the shared ATGM336H GNSS UART/TinyGPSPlus parser and starts only the GNSS serial path.
- Saves one home waypoint to Preferences/NVS under `scoober_home` when the user presses `S` with a fresh GNSS fix.
- Clears the saved home waypoint with `D`.
- Shows live distance and bearing from the current fresh GNSS fix back to the saved home point.
- Converts the return bearing to a compass direction using the existing GNSS compass helper.
- Shows an arrival status once the current position is within `RETURN_HOME_ARRIVAL_RADIUS_METERS = 10.0f` of the saved home point.
- Priority #8 OLED behavior: `Return Home` shows a compact 5x7 banner, saved-home navigation description, `S`, `D`, OK/`R`, Backspace commands, distance, and bearing.
- Starts only the GNSS serial parser; it does not initialize SX1262, claim the shared SPI bus, or transmit.
- OK/Enter or R restarts the parser without deleting the saved home point.
- Backspace returns to the main menu and stops the GNSS serial object.
- No transmit path is enabled in this milestone.
- User confirmed `Return Home` is working great on hardware on 2026-08-29.

Controls:

- `S`: save/update the current fresh GNSS fix as home.
- `D`: clear the saved home point.
- OK/Enter or `R`: restart the shared GNSS parser.
- Backspace: return to the main menu.

Guard:

- `tools/check_return_home.py`

### Breadcrumb Logger

Behavior:

- Adds `Breadcrumbs` as a Priority #11 Cap LoRa-1262 feature-expansion screen.
- Reuses the shared ATGM336H GNSS UART/TinyGPSPlus parser in `src/lora_gnss.cpp`.
- Starts only the GNSS serial parser; it does not initialize SX1262, claim the shared SPI bus for LoRa, or transmit.
- Creates `/tracks` on microSD when needed and writes CSV logs as `/tracks/trackNNN.csv`.
- Writes `BREADCRUMB_LOG_HEADER` once, then appends fresh GNSS fixes every `BREADCRUMB_LOG_SAMPLE_INTERVAL_MS = 5000`.
- Logs uptime seconds, UTC, date, latitude, longitude, satellites, HDOP, speed, and altitude.
- Skips samples when the GNSS fix is missing or stale and shows the missed-fix count.
- Flushes each row after writing so short walks still leave usable data if power is interrupted.
- Priority #8 OLED behavior: `Breadcrumbs` shows a compact 5x7 banner, GNSS CSV track-log description, `S`, OK/`R`, Backspace commands, fresh-fix interval, point/missed counts, and file hint.
- Backspace returns to the main menu and closes the active CSV file if logging is running.
- No transmit path is enabled in this milestone.

Controls:

- `S`: start or stop CSV track logging.
- OK/Enter or `R`: restart the shared GNSS parser and close any active log.
- Backspace: return to the main menu.

Guard:

- `tools/check_breadcrumb_logger.py`

### LoRa Packet Monitor

Behavior:

- Adds `LoRa Packets` as a Priority #11 Cap LoRa-1262 feature-expansion screen.
- Uses RadioLib for the SX1262 LoRa radio and the same known-good RX settings as `LoRa Diag`.
- Detects the Cap LoRa-1262 PI4IOE5V6408 I/O expander at `0x43`.
- Enables the required antenna switch by setting PI4IOE5V6408 `P0` high.
- Acts as an RX-only packet viewer for matching LoRa settings.
- Shows packet count, CRC mismatch count, receive error count, payload preview, payload length, age, RSSI, SNR, frequency, spreading factor, and bandwidth.
- Before a packet is decoded, labels live RSSI as channel/noise RSSI and leaves SNR, payload length, age, and payload preview in a no-packet state.
- Packet count, packet RSSI, SNR, payload length, age, and payload preview only update after a transmitter sends matching LoRa frames.
- Sanitizes non-printable packet payload bytes for display and clips the preview to `LORA_PACKET_MONITOR_PAYLOAD_MAX_CHARS = 64`.
- Priority #8 OLED behavior: `LoRa Packets` shows a compact 5x7 banner and command sheet for `C`, OK/`R`, Backspace, matching-sender hint, packet/CRC/error summary, and RX-only/no-TX status.
- Backspace returns to the main menu and puts the SX1262 radio to sleep.
- No transmit path is enabled in this milestone.

Controls:

- `C`: clear packet counters and payload preview without restarting the radio.
- OK/Enter or `R`: restart the RX-only packet monitor.
- Backspace: return to the main menu.

Guard:

- `tools/check_lora_packet_monitor.py`

### LoRa TX Planning

Status:

- LoRa TX planning started on 2026-09-13 and is captured in [docs/superpowers/plans/2026-09-13-lora-tx-planning.md](docs/superpowers/plans/2026-09-13-lora-tx-planning.md).
- This is a planning gate only; no active Cardputer transmit firmware has been added yet.
- The separate XIAO ESP32-S3 Wio-SX1262 LoRa ACK Node scaffold lives in [nodes/xiao_sx1262_lora_ack](nodes/xiao_sx1262_lora_ack) so the first range-test work has a second-node bench partner.
- Current RX-only screens (`LoRa Diag` and `LoRa Packets`) must stay no-transmit/RX-only.

Planning assumptions:

- Current operating assumption is US 902-928 MHz, matching the user location and the current 915.0 MHz RX monitor settings.
- Antenna must be installed before any TX test; use the included Cap LoRa-1262 SMA antenna first.
- Start with the current known-good radio settings: 915.0 MHz, 125 kHz, SF12, CR 4/5, sync word 0x34, and preamble 20.
- Start TX power at 2 dBm, matching the existing RX init placeholder, then treat any power increase as a deliberate later decision.
- Keep manual TX rate limited and explicitly armed because firmware cannot detect antenna attachment.
- For the companion node, use Seeed's XIAO ESP32S3 + Wio-SX1262 B2B defaults: `GPIO41` NSS, `GPIO39` DIO1/IRQ, `GPIO42` RST, `GPIO40` BUSY, `GPIO38` RF switch, and XIAO default SPI pins `GPIO7/GPIO8/GPIO9`.
- If the received module differs from the B2B version, override the `XIAO_LORA_*_PIN` build flags before upload.

First TX-capable feature:

- Implement `LoRa Ping / Range Test` before pager, beacon, MQTT bridge TX, or signal-map automation.
- Use canned ASCII payloads, the XIAO ESP32-S3 Wio-SX1262 LoRa ACK Node, and short ACK packets before attempting free-form text or periodic beacons.
- The XIAO manual probe is `SCBR,NODE,1,xiao-sx1262-ack,<seq>,<uptime_ms>` and has no periodic beacons.
- The XIAO ping reply is `SCBR,ACK,1,xiao-sx1262-ack,<seq>,<remote_rssi>,<remote_snr>,<remote_uptime_ms>`.
- Log range-test CSV rows as `/tracks/lora-rangeNNN.csv` so the existing SD Manager can inspect the results without adding another folder first.

References:

- eCFR 47 CFR 15.5 general conditions: https://www.ecfr.gov/current/title-47/chapter-I/subchapter-A/part-15/subpart-A/section-15.5
- eCFR 47 CFR 15.23 home-built devices: https://www.ecfr.gov/current/title-47/chapter-I/subchapter-A/part-15
- eCFR 47 CFR 15.247 902-928 MHz intentional radiators: https://www.ecfr.gov/current/title-47/chapter-I/subchapter-A/part-15/subpart-C/subject-group-ECFR2f2e5828339709e/section-15.247
- eCFR 47 CFR 15.249 902-928 MHz field-strength option: https://www.ecfr.gov/current/title-47/chapter-I/subchapter-A/part-15/subpart-C/subject-group-ECFR2f2e5828339709e/section-15.249
- M5Stack Cap LoRa-1262 product page: https://shop.m5stack.com/products/cap-lora-1262-for-cardputer-adv-sx1262-atgm336h
- Seeed Studio Wio-SX1262 for XIAO product page: https://www.seeedstudio.com/Wio-SX1262-for-XIAO-p-6379.html
- Seeed Studio Wio-SX1262 introduction: https://wiki.seeedstudio.com/wio_sx1262/
- Seeed Studio XIAO ESP32S3 & Wio-SX1262 kit introduction: https://wiki.seeedstudio.com/wio_sx1262_with_xiao_esp32s3_kit/

Guard:

- `tools/check_lora_tx_planning.py`
- `tools/check_xiao_sx1262_node.py`

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
| DS3231 / AT24C32 RTC via PaHub | Channel 5 | Priority #9 RTC status screen; DS3231 `0x68`, AT24C32 `0x57` |
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
  - RTC selects the DS3231 / AT24C32 module on PaHub channel 5 and probes `0x68` / `0x57`.
  - Internal I2C is used by Cardputer hardware through M5 libraries.
  - Cap LoRa-1262 uses internal I2C G8/G9 for the PI4IOE5V6408 antenna-switch expander at `0x43`.
- **SPI**
  - Used for microSD access.
  - Cap LoRa-1262 SX1262 diagnostics also use EXT SPI pins `G40/G39/G14` with `G5 NSS`; microSD remains on `G12 CS`.
  - `prepareSharedSpiForSd()` and `deselectSharedSpiDevices()` coordinate handoff between LoRa and SD-backed features.
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
- Press OK on the selected `set_interval` command to publish `{"command":"set_interval","seconds":<selected>}`.
- Press `S` to advance the OLED MQTT message page manually.
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
- RTC time is displayed as the DS3231's stored local module time. NTP setting currently uses Mountain time; timezone configuration and timestamp consumers are not active yet.

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

1. Continue Priority #11 with the LoRa TX planning gate in [docs/superpowers/plans/2026-09-13-lora-tx-planning.md](docs/superpowers/plans/2026-09-13-lora-tx-planning.md).
2. Build and hardware-check the XIAO ESP32-S3 Wio-SX1262 LoRa ACK Node in [nodes/xiao_sx1262_lora_ack](nodes/xiao_sx1262_lora_ack) once the module arrives.
3. Confirm the TX checklist before Cardputer firmware work: antenna installed, US 902-928 MHz region, 915.0 MHz / 125 kHz / SF12 / CR 4/5 / sync word 0x34 / preamble 20, 2 dBm initial TX power, explicit arm state, canned ASCII payloads, rate limit, and a second node for ACK testing.
4. Keep `LoRa Diag` and `LoRa Packets` RX-only while adding any dedicated TX feature.
5. Implement `LoRa Ping / Range Test` before pager, beacon, MQTT bridge TX, or signal-map automation.
6. Log first range-test CSV rows to `/tracks/lora-rangeNNN.csv` so SD Manager can inspect them.
7. Run `tools/check_lora_tx_planning.py`, `tools/check_xiao_sx1262_node.py`, and the full guard suite before and after any TX-capable firmware change.

Good near-term improvements:

1. Make the RTC timezone configurable, likely from SD config, if the device needs to travel outside Mountain time.
2. Use RTC time for Environment logs, Voice Memo names, OLED clock/status, and Pi Monitor timestamps where it helps.
3. Revisit `Resp:` only if future Pi Monitor commands need explicit success/failure acknowledgments; `Last` / `Pay` are working for now.
4. Add MQTT authentication after live Mosquitto configuration is confirmed.

Future bigger milestones:

1. Scoober Pager after LoRa Ping / Range Test is stable.
2. Location Beacon after explicit privacy, airtime, and interval choices.
3. MQTT LoRa Bridge with loop prevention and whitelisted Wi-Fi-to-LoRa actions.
4. Signal Map using GNSS position plus known-beacon RSSI/SNR.
5. Broader Raspberry Pi command center integration.
6. More hardware tools using IR, Grove, BLE, or other Cardputer expansion options.

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
- Voice memo mic board/channel/codec probe diagnostics
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

Build XIAO SX1262 LoRa ACK node:

```sh
python -m platformio run -d nodes/xiao_sx1262_lora_ack
```

Upload:

```sh
python -m platformio run --target upload
```

Upload XIAO NRF24 OLED node:

```sh
python -m platformio run -d nodes/xiao_nrf24_oled --target upload
```

Upload XIAO SX1262 LoRa ACK node:

```sh
python -m platformio run -d nodes/xiao_sx1262_lora_ack --target upload
```

Size:

```sh
python -m platformio run --target size
```

Run all guard scripts:

```sh
python tools/check_battery_trend.py
python tools/check_breadcrumb_logger.py
python tools/check_display_refresh.py
python tools/check_environment_feature.py
python tools/check_external_display_revisit.py
python tools/check_level_tool.py
python tools/check_lora_cap_diag.py
python tools/check_lora_gnss_dashboard.py
python tools/check_lora_gnss_sky_view.py
python tools/check_lora_packet_monitor.py
python tools/check_lora_tx_planning.py
python tools/check_menu_structure.py
python tools/check_nrf24_feature.py
python tools/check_oled_test.py
python tools/check_oled_status_dashboard.py
python tools/check_pi_command_center_plan.py
python tools/check_power_status.py
python tools/check_return_home.py
python tools/check_rtc_status.py
python tools/check_sd_manager.py
python tools/check_saved_wifi.py
python tools/check_wifi_credentials_strategy.py
python tools/check_voice_memos.py
python tools/check_wifi_scroll.py
python tools/check_xiao_nrf24_node.py
python tools/check_xiao_sx1262_node.py
```

Search code quickly:

```sh
rg "Environment|Voice Memos|Battery|WiFi|RTC|LoRa|GNSS|Breadcrumb|NRF24" src tools README.md notes.md todo.md
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
- Pi Monitor opens on a project list with `Home / Diagnostics`, configured projects, and discovered device IDs.
- Pi Monitor opens a command list for the selected project and uses the OLED for a one-header, manually paged, project-filtered MQTT message viewer.
- Pi Monitor accepts `project=id|Label|profile` lines in `/config/pi.txt`; `command_target=esp32-c3-test` remains compatible.
- Pi Monitor publishes Cardputer availability to `home/devices/scoober-cardputer/availability` and status JSON to `home/devices/scoober-cardputer/status`.
- User confirmed Cardputer MQTT status/availability publishing works on hardware on 2026-08-15.
- Pi Monitor publishes `read_now` to `home/devices/<command_target>/commands` with `C` when `command_target=esp32-c3-test` is present in `/config/pi.txt`.
- Pi Monitor cycles command targets with `T`.
- User confirmed Pi Monitor target selection works on hardware on 2026-08-15.
- Pi Monitor cycles fixed `set_interval` choices with `I`, publishes the selected command with OK, and advances the OLED MQTT message page with `S`.
- User confirmed Pi Monitor project/command UI and manual `S` OLED MQTT message paging work on hardware on 2026-09-05.
- Pi Monitor shows response topics such as `home/devices/<device>/responses`, nested response topics, and post-command `command_target` updates on a dedicated `Resp:` line.
- User reported on 2026-08-06 that `Resp:` still stayed on `waiting`; this is not blocking while `Last` / `Pay` and telemetry show command results.
- Pi Monitor disconnects MQTT with `D`, clears/reconnects with `R`, and retries connection with OK/Enter.
- User confirmed Pi Monitor MQTT viewing works on Cardputer hardware on 2026-07-29.
- User confirmed Pi Monitor `read_now` command publishing works on Cardputer hardware on 2026-07-30.
- SD Manager opens from the main menu and lists `/config`, `/env`, `/memos`, and `/tracks`.
- SD Manager can browse `/config`, `/env`, `/memos`, and `/tracks` without touching other SD paths.
- SD Manager views small text files, previews the first page of large CSV logs, and treats WAV, large non-CSV text, and unknown binary files as view/delete-only.
- SD Manager edits `/config/wifi.txt` and `/config/pi.txt` through the key/value editor while preserving Wi-Fi and Pi Monitor config formats.
- SD Manager saves edited configs with a temp-file save followed by rename.
- SD Manager requires confirmation before create, rename, or delete actions.
- SD Manager refuses SD access while Voice Memos is recording or Environment logging is active.
- SD Manager shows SD card info with `I` from the folder list.
- SD Manager can create confirmed `/config/wifi.txt` and `/config/pi.txt` templates with `W` and `P` from `/config`.
- User confirmed the Priority #12 hardware checklist and polish spot-checks passed on 2026-09-08.
- Voice Memos records WAV files to microSD.
- Voice Memos lists saved memos, plays with OK/Enter, and deletes selected memos with `D`.
- Environment shows live temperature, humidity, and pressure when ENV III is connected.
- Environment shows a not-found/retry message when ENV III is disconnected.
- Environment uses `L to name and start logging`; OK/Enter starts logging and Backspace deletes characters while naming.
- Environment writes `/env/env001.csv` or named files such as `/env/backyard001.csv` to microSD.
- Environment ignores invalid pressure readings instead of logging impossible values.
- RTC shows DS3231 status, date, time, oscillator-stopped warning, temperature, AT24C32 presence, and read count when the module is on PaHub channel 5.
- RTC sets/corrects the DS3231 from NTP local time with `N` after Wi-Fi is connected through WiFi Connect.
- RTC keeps `S` as an offline build-time fallback and reports missing RTC hardware gracefully.
- RTC retries detection/read with OK/Enter or `R`, and the read count increments so the retry is visible.
- GNSS Dash shows fix/no-fix, satellites, HDOP, coordinates, speed, altitude, UTC/date, NMEA lines, checksum counts, and byte count without LoRa transmit behavior.
- GNSS Sky shows GSV count/age, satellites-in-view, plotted satellite dots, and strongest SNR satellite on the LCD summary panel.
- GNSS Sky arrow keys cycle the selected satellite, skip stale entries, highlight the selected dot, and show selected label, constellation, PRN, SNR, elevation, azimuth, compass direction, and age on the OLED.
- Return Home saves/updates a fresh GNSS fix with `S`, shows distance and bearing back to the saved home point, and clears the saved home point with `D`.
- Breadcrumbs starts/stops CSV track logging with `S`, creates `/tracks/trackNNN.csv`, logs fresh GNSS fixes every 5 seconds, and closes the file on Backspace.
- Breadcrumbs skips stale/no-fix samples and increments the missed-fix count instead of logging bad coordinates.
- LoRa Packets starts an RX-only packet monitor using the same frequency/bandwidth/SF as LoRa Diag, shows packet/CRC/error counts, RSSI, SNR, payload length, and a clipped payload preview.
- LoRa Packets shows channel/noise RSSI before the first packet; `Pk`, packet SNR, length, age, and payload remain in the no-packet state until a matching LoRa transmitter is present.
- LoRa Packets clears counters with `C`, restarts with OK/Enter or `R`, and never exposes a transmit action.
- Priority #8 OLED command/help sheets appear on WiFi Connect, first-entry Pi Monitor, SD Manager, RTC, GNSS Dash, GNSS Sky, Return Home, Breadcrumbs, LoRa Packets, and LoRa Diag; GNSS Sky omits the banner and keeps selected-satellite data visible.
- Menu, Battery, System, WiFi Scan, Saved WiFi, Voice Memos, Environment, OLED Test, and Level retain their existing OLED behavior for now.
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
- If OLED Test says `OLED not found`, check the `Scan:` line for the last channel 1 `0x3C` / `0x3D` probe errors and retry with OK/Enter or R.
- If OLED Test shows `Bus:100k` and the OLED works, leave it on the slower fallback for now; the firmware will keep using that speed for OLED drawing.
- If OLED Test says `OLED not found`, confirm the OLED is on PaHub channel 1 and try OK/Enter or R.
- A persistent OLED `not found` report with PaHub/ENV/RTC still working was fixed on 2026-09-05 by replacing the Grove cable.
- If RTC says `PaHub ch5 missing`, confirm the PaHub input is connected, the DIP switch is at `0x70`, and the RTC module is on PaHub channel 5.
- If RTC says `DS3231 not found`, confirm the module power/wiring and use OK/Enter or R to retry detection.
- If Voice Memos shows SD errors, confirm the microSD card is inserted and formatted.
- If needed, restore factory firmware with M5Burner.
