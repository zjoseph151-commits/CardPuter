# Scoober Technical Notes

These notes preserve project context for future Codex sessions. They are intentionally detailed and include decisions, experiments, and hardware lessons that are easy to lose in chat history.

## Project Identity

- Firmware/project name shown to the user: `Scoober`
- Original scaffold idea: `cardputer-project-launcher`
- Current firmware version constant: `v0.1.0`
- Firmware entry point: `src/main.cpp`
- Target board in PlatformIO: `m5stack-stamps3`
- Framework: Arduino
- Main PlatformIO platform: pioarduino `platform-espressif32` pinned to `54.03.21`
- Main Arduino-ESP32 / ESP-IDF stack: Arduino-ESP32 `3.2.1`, ESP-IDF `5.4.x`
- Main environment overrides `tool-esptoolpy` to `platformio/tool-esptoolpy@1.40801.0` because the older pinned platform's bundled esptool package is not compatible with the current Python 3.13/click runtime. `tools/esptool_compat.py` normalizes the pinned pioarduino platform's esptool command names, flash option flags, and reset mode values to the PlatformIO registry esptool CLI.
- Device: M5Stack Cardputer Adv Version

## Source Layout

- `src/main.cpp`: `setup()` and `loop()`.
- `src/app.h`: shared constants, types, global declarations, and function prototypes.
- `src/app_state.cpp`: shared global state, menu definitions, and hardware helper objects.
- `src/ui.cpp`: screen routing, frame/content drawing helpers, and main menu rendering.
- `src/input.cpp`: keyboard event dispatch and screen-specific key handling.
- `src/i2c_hub.cpp`: M5Stack Unit PaHub v2.1 detection and channel selection for shared Grove I2C.
- `src/power_screen.cpp`: Battery/System screens and battery trend logic.
- `src/wifi_connect.cpp`: SD-backed Wi-Fi credential reading and connect/disconnect screen.
- `src/wifi_screens.cpp`: Wi-Fi scan, saved SSID list, save/delete flows, and Preferences storage.
- `src/pi_monitor.cpp`: SD-backed Raspberry Pi MQTT config, MQTT subscriptions, project list / command list UI, Cardputer status/availability publisher, target selection, scoped MQTT message history, and whitelisted `read_now` / `set_interval` command publishing.
- `src/sd_manager.cpp`: SD Manager for fixed-folder microSD browsing, guarded config editing, config template creation, SD card info, and confirmed create/rename/delete operations.
- `src/voice_memos.cpp`: microSD WAV recording, listing, playback, and delete flow.
- `src/environment_screen.cpp`: ENV III readings and CSV logging.
- `src/oled_test.cpp`: SSD1309 OLED Status Dashboard, Priority #8 compact command/help sheets, and OLED Test diagnostics on PaHub channel 1.
- `src/rtc_status.cpp`: Priority #9 DS3231 / AT24C32 RTC status screen on PaHub channel 5.
- `src/lora_gnss.cpp`: shared M5Stack Cap LoRa-1262 ATGM336H GNSS UART and TinyGPSPlus parser.
- `src/gnss_dashboard.cpp`: `GNSS Dash` Priority #11 screen showing parsed GNSS details without starting the LoRa radio.
- `src/gnss_sky_view.cpp`: `GNSS Sky` Priority #11 screen drawing GSV satellite elevation/azimuth/SNR data and selected satellite highlighting.
- `src/return_home.cpp`: `Return Home` Priority #11 screen saving one home waypoint and showing GNSS distance/bearing back to it without starting the LoRa radio.
- `src/breadcrumb_logger.cpp`: `Breadcrumbs` Priority #11 screen writing fresh GNSS fixes to `/tracks/trackNNN.csv` on microSD without starting the LoRa radio.
- `src/lora_diag.cpp`: paged RX-only M5Stack Cap LoRa-1262 packet and hardware/GNSS diagnostics.
- `src/lora_messages.cpp`: manual short-message TX/RX with matched ACKs and in-memory history.
- `src/shared_spi.cpp`: shared external SPI chip-select and owner handoff helper for LoRa/microSD sharing.
- `src/level_tool.cpp`: BMI270 level/crosshair tool.
- `docs/superpowers/plans/2026-09-13-lora-tx-planning.md`: LoRa TX planning gate before Priority #11 transmit-capable firmware work.
- Firmware guard scripts use `tools/firmware_source.py` so checks scan all `.cpp` and `.h` files under `src`.

## Current Menu State

The active menu is defined in `MENU_ITEMS`:

```cpp
{"Battery", Screen::BatteryInfo}
{"System", Screen::SystemInfo}
{"WiFi Scan", Screen::WifiScan}
{"Saved WiFi", Screen::SavedWifi}
{"WiFi Connect", Screen::WifiConnect}
{"Pi Monitor", Screen::PiMonitor}
{"SD Manager", Screen::SdManager}
{"Voice Memos", Screen::VoiceMemos}
{"Environment", Screen::Environment}
{"OLED Test", Screen::OledTest}
{"RTC", Screen::RtcStatus}
{"GNSS Dash", Screen::GnssDashboard}
{"GNSS Sky", Screen::GnssSkyView}
{"Return Home", Screen::ReturnHome}
{"Breadcrumbs", Screen::BreadcrumbLogger}
{"LoRa Diag", Screen::LoraDiag}
{"LoRa Messages", Screen::LoraMessages}
{"Level", Screen::LevelTool}
```

Removed menu items:

- Keyboard Test
- Display Test
- SD Card Test
- ESP-NOW RC Placeholder
- IMU Test, replaced by Level

## Input Handling Notes

The Cardputer keyboard reports arrow-like navigation through character values in `keys.word`.

Current navigation mapping:

- Up: `;` or `,`
- Down: `.` or `/`
- Select: `keys.enter`
- Back/menu: `keys.del`

Feature-specific keys:

- WiFi Scan: `R` rescans, OK saves selected network name
- Saved WiFi: `D` deletes selected saved SSID after confirmation
- WiFi Connect: OK retries connection, `D` disconnects
- Pi Monitor: arrows scroll the project list or command list, OK opens the highlighted project or sends the highlighted command, Home / Diagnostics OK retries MQTT, Backspace returns from a Pi Monitor subview to the project list, `C` publishes whitelisted `read_now`, `T` cycles command targets, `I` cycles fixed intervals for the selected `set_interval` command, `S` advances the OLED MQTT message page, `R` clears the device list and reconnects, `D` disconnects MQTT
- Voice Memos: `R` records/stops, OK plays, `D` deletes after confirmation
- OLED Test: OK/Enter or `R` retries OLED detection
- RTC: `N` sets/corrects the DS3231 from NTP local time when Wi-Fi is already connected through WiFi Connect; `S` remains an offline build-time fallback; OK/Enter or `R` retries DS3231 / AT24C32 detection and read on PaHub channel 5
- GNSS Dash: OK/Enter or `R` restarts the shared GNSS parser
- GNSS Sky: arrow keys cycle the selected satellite through active plotted satellites; OK/Enter or `R` restarts the shared GNSS parser
- Return Home: `S` saves/updates the current fresh GNSS fix as the saved home point, `D` clears the saved home point, OK/Enter or `R` restarts the shared GNSS parser
- Breadcrumbs: `S` starts/stops CSV track logging to `/tracks/trackNNN.csv`; OK/Enter or `R` restarts the shared GNSS parser and closes any active log
- LoRa Diag: arrows change packet/hardware pages, `C` clears packet counters/payload preview, OK/Enter or `R` restarts RX-only diagnostics
- LoRa Messages: `N`/OK composes; Enter sends; Backspace erases or cancels an empty draft; arrows browse history; `R` restarts the radio

Do not re-add the old footer text inside every feature. The user asked to remove it.

## Display Refresh Notes

Earlier dynamic screens flickered because the whole screen/header was redrawn on every update.

Current pattern:

- Call `drawScreenFrame(title)` when entering a screen.
- For dynamic content, call `beginContentDraw()`, draw to `contentCanvas`, then `commitContentDraw()`.
- `commitContentDraw()` pushes the sprite to `(0, CONTENT_TOP)` so the header remains stable.
- `initContentCanvas()` owns sprite allocation, forces non-PSRAM allocation, and tries 8-bit color if the normal 16-bit canvas fails.
- `beginContentDraw()` retries missing canvas allocation and `commitContentDraw()` skips `pushSprite()` if the buffer is still missing, preventing reset loops when feature screens open.

Guard:

- `tools/check_display_refresh.py`

Dynamic screens that should avoid full header redraw:

- Battery
- System
- Environment
- Level

## Battery And Charging Notes

The Cardputer power API did not provide enough reliable charging information for the desired UI.

Observed behavior:

- Not charging can fluctuate around +/-10 mV.
- Unplugged drift has also been observed sitting around +26 mV.
- Charging can trend around +100 mV.
- Raw current can show `0mA` or unavailable even while the battery percentage rises.
- Raw charge status can be unknown.
- Battery percentage can jump while plugged in because it appears to be voltage-derived.

Current approach:

- Sample battery continuously in `loop()`.
- Keep trend alive across screen changes.
- Show only `Charging` or `Not charging`.
- Use `CHARGING_TREND_THRESHOLD_MV = 50`.
- Use `CHARGING_CONFIRM_SAMPLES = 3`.
- Use `CHARGING_CLEAR_SAMPLES = 3`.
- Use `VBUS_PRESENT_THRESHOLD_MV = 4500`.
- Clear inferred charging after a confirmed voltage drop from the sampled peak, then restart the trend baseline at the current sample.
- Do not use battery percentage increases as charging evidence.
- Do not show `Charging` unless VBUS/external power is present.

Important history:

- Resetting trend only when entering Battery caused incorrect readings when plugging/unplugging on the main menu.
- A +20 mV charging threshold was too low because unplugged drift could sit around +26 mV.
- ENV III attached to Grove can disturb raw Battery readings enough that Battery now requires VBUS before showing `Charging`.
- Labels like `Watching`, `Likely charging`, and similar were rejected by the user.
- Firmware version was removed from Battery because System already shows it.

Guard:

- `tools/check_power_status.py`
- `tools/check_battery_trend.py`

## Wi-Fi Notes

Wi-Fi has scan, SSID-only saved names, and an SD-backed connect screen.
User confirmed the WiFi Connect screen tested successfully on Cardputer hardware on 2026-07-23.

Current rules:

- Credentials must come from microSD `/config/wifi.txt`.
- `WiFi.begin` belongs only in the intentional WiFi Connect flow.
- No hardcoded credentials.
- No Wi-Fi passwords in Preferences/NVS.
- Saved WiFi stores only SSID names.
- Saved names survive reboot through Preferences/NVS.

Credential strategy:

- Use a microSD file at `/config/wifi.txt`.
- Format:
  - `ssid=YourNetworkName`
  - `password=YourNetworkPassword`
- Do not hardcode credentials in source code.
- Do not store Wi-Fi passwords in Preferences/NVS.
- Do not commit real `wifi.txt` files; `.gitignore` excludes `/config/wifi.txt` and `/wifi.txt` for accidental local copies.
- The WiFi Connect screen shows status, uses a 15 second timeout, handles missing SD/config gracefully, and returns safely to the menu.
- OK/Enter retries the connection; `D` disconnects.
- Successful connection displays the local IP address.

Preferences namespace:

- `scoober_wifi`

Saved SSID keys:

- `count`
- `ssid00`, `ssid01`, etc.

Caps:

- `MAX_WIFI_NETWORKS = 40`
- `WIFI_VISIBLE_ROWS = 5`
- `MAX_SAVED_WIFI_NAMES = 20`
- `SAVED_WIFI_VISIBLE_ROWS = 5`

Guards:

- `tools/check_wifi_scroll.py`
- `tools/check_saved_wifi.py`
- `tools/check_wifi_credentials_strategy.py`

## Voice Memo Notes

Voice memo feature uses the built-in mic/speaker and microSD.

Storage:

- Directory: `/memos`
- File names: `memo001.wav` through `memo999.wav`
- Format: WAV, PCM, 16-bit, mono
- Sample rate: 16000 Hz
- Max recording time: 30 seconds
- Record chunk size: 240 samples

Pins:

- SD SCK: G40
- SD MISO: G39
- SD MOSI: G14
- SD CS: G12

Implementation notes:

- Recording writes a placeholder WAV header first and patches it when recording stops.
- M5Unified `Mic.record()` queues asynchronous capture into the provided buffer. Do not write a newly queued buffer to SD immediately; wait until the chunk is no longer recording and the capture window has elapsed, then write the filled buffer.
- The Voice Memos recording screen shows `peak:` and a `Mic ...` status on the LCD/OLED so hardware tests can distinguish a silent mic/input path from an SD write problem.
- Recording probes right/left/stereo input channels, chooses the loudest working channel, and labels quiet chunks when the peak stays at or below the observed floor threshold.
- The Cardputer Adv mic path depends on the ES8311 codec at internal I2C address `0x18`. Voice Memos restores the Adv audio pins (`G46` data, `G43` WS/LRCK, `G41` BCLK), enables the ES8311 ADC path, and prints board/config/codec/probe details to Serial when recording starts.
- Non-Adv Cardputer keeps the built-in PDM mic path on data `G46` / WS `G43` with no BCLK.
- Hardware testing showed all right/left/stereo probes stuck at `peak:8` while ES8311 was visible and M5Unified reported the mic task running. This matches the known Cardputer Adv silent-mic behavior under the ESP-IDF 5.5 I2S stack, so the main PlatformIO environment is pinned to pioarduino `54.03.21` / ESP-IDF `5.4.x`.
- If no audio bytes are recorded, the file is deleted.
- Playback validates RIFF/WAVE basics and only supports 16-bit mono PCM.
- Playback is simple/blocking.
- Speaker is ended before recording; mic is ended before playback.

Guard:

- `tools/check_voice_memos.py`

## Environment Feature Notes

Hardware:

- M5Stack ENV III Unit
- Uses SHT30 for temperature/humidity
- Uses QMP6988 for pressure

Library:

- `m5stack/M5Unit-ENV`
- Include: `M5UnitENV.h`

Pins:

- `ENV_I2C_SDA_PIN = 2`
- `ENV_I2C_SCL_PIN = 1`
- `ENV_I2C_FREQUENCY = 400000`
- Uses the global Arduino `Wire` object.

Timing:

- Refresh: 1000 ms
- Retry when missing: 3000 ms
- Logging writes one row per fresh Environment reading while active.

Behavior:

- Initializes both SHT30 and QMP6988.
- Allows partial success.
- If neither responds, displays `ENV III not found`.
- Retries while the screen is open.
- Prints readings/status to Serial.
- Pressure is accepted only when finite and between `300.0` and `1100.0` hPa.
- Invalid pressure displays as `Pressure: invalid` and is left blank in CSV logs.
- Invalid pressure triggers periodic ENV III reinitialization in case QMP6988 calibration was read badly through the PaHub.
- Altitude was removed from the screen and CSV logs because it was not useful and became `nan` when QMP6988 returned bad pressure.
- Press `L` on the Environment screen to open a log-name entry screen before starting optional CSV logging.
- OK/Enter starts logging with the typed name; Backspace deletes characters, or cancels when the name is blank.
- Names allow letters, numbers, spaces, `_`, and `-`; spaces are saved as underscores and names are capped at 16 characters.
- Blank names fall back to `env001.csv`, `env002.csv`, etc.
- Named sessions create files in `/env`, such as `backyard001.csv`.
- CSV header: `uptime_s,temp_c,temp_f,humidity_pct,pressure_hpa`.
- The screen shows the active log file name and sample count.
- Backspace stops any active Environment log before returning to the main menu.
- Missing SD card or `/env` creation failure shows a status message and leaves the firmware usable.

Important history:

- A `TwoWire envWire(1)` experiment compiled, but on hardware QMP6988 pressure read as `0.0 hPa` and altitude as `inf m`.
- Reverted ENV III to the global `Wire` path because it reads both SHT30 and QMP6988 correctly.
- On 2026-08-15 with ENV III through PaHub channel 0, user reported pressure around `-3300 hPa` and altitude `nan`; pressure sanity filtering was added and altitude was removed.

Manual wiring if not using Grove connector:

- VCC -> Grove 5V
- GND -> GND
- SDA -> Grove SDA
- SCL -> Grove SCL

Guard:

- `tools/check_environment_feature.py`

## NRF24L01 / RF Scan Removal Notes

NRF24L01 / RF Scan feature was removed from active firmware on 2026-08-25.

Reason:

- The Cardputer RF Scan path used the Cardputer Adv EXT SPI pins.
- The upcoming M5Stack Cap LoRa-1262 also uses the EXT interface for SX1262 LoRa and ATGM336H GNSS work.
- The active firmware should keep that EXT path clear for Cap LoRa-1262 diagnostics.

Removal scope:

- Main firmware no longer has `src/rf_scanner.cpp`.
- Main menu no longer includes `RF Scan`.
- `Screen::RfScanner`, RF Scan keyboard handling, and RF Scan OLED dashboard lines were removed.
- Main `platformio.ini` no longer depends on `nrf24/RF24`.
- `tools/check_nrf24_feature.py` now guards the removal.

## Retired XIAO Two-Node Findings

The XIAO proof node is still in the repo for future investigation, but it is no longer the active Cardputer feature.

Observed behavior:

- Cardputer pings reached the XIAO node.
- XIAO `TX` and `OK` increased, and `F` stayed at 0.
- Cardputer did not receive `XIAO beacon N` or `XIAO ack N`.
- Cardputer RF RPD/carrier counters moved during ping tests, but valid packets did not decode.
- This points away from an application protocol issue and toward module power, PA/LNA behavior, receiver sensitivity, antenna/module mismatch, or a one-way hardware issue.

Future bench steps if we revisit it:

- Swap the NRF24 modules between Cardputer and XIAO.
- Power the XIAO NRF24 adapter from a stable external supply with a common ground.
- Add a local capacitor near the NRF24 adapter power pins.
- Try a known-good third NRF24 node.
- Compare PA/LNA modules against basic non-PA NRF24 modules.

XIAO test node:

- Location: `nodes/xiao_nrf24_oled`
- Board: `seeed_xiao_esp32c3`
- Uses a separate PlatformIO project so it can build/upload independently from the Cardputer firmware.
- OLED: SSD1306 128x64 over I2C, SDA G6, SCL G7.
- NRF24 SPI:
  - SCK: D9 / G9
  - MISO: D6 / G21
  - MOSI: G10
  - CE: D7 / G20
  - CSN: D8 / G8
- XIAO uses the same shared `SCBR1` address as the Cardputer.
- XIAO sends `XIAO beacon N` about four times per second.
- XIAO uses `RF24_PA_MIN` during this proof test to reduce transmit current draw from the PA/LNA module.
- XIAO OLED separates TX OK/fail counts from TX attempts and shows the last RX pipe/FIFO diagnostics.
- XIAO replies to received Cardputer packets with three `XIAO ack N` packets.
- XIAO waits `REPLY_DELAY_MS = 200` before replying so the Cardputer is back in listen mode.
- XIAO OLED shows radio status, TX/RX counters, and last packet text.

Guard:

- `tools/check_xiao_nrf24_node.py`

## Level Tool Notes

The old IMU Test was changed into a level tool.

Hardware:

- Cardputer Adv built-in BMI270 IMU

Behavior:

- Updates every ~80 ms.
- Draws crosshair and dot.
- Green center tolerance means level.
- Shows pitch and roll.

Constants:

- `LEVEL_DOT_SCALE_PIXELS = 70.0f`
- `LEVEL_TOLERANCE_PIXELS = 6`
- `LEVEL_SMOOTHING = 0.25f`

Guard:

- `tools/check_level_tool.py`

## OLED Experiment Notes

The user bought/planned a 2.42 inch SSD1309 OLED, paused it after the first pin-conflict experiment, then brought it back through the PaHub path.

What was tried:

- Added U8g2 dependency and OLED Test screen.
- Tried using G8/G9 to preserve Grove.
- Arrow keys stopped working.
- Investigation showed G8/G9 are Cardputer Adv internal I2C lines, used by onboard hardware including keyboard path.
- Moved OLED to Grove G2/G1; test worked.
- User decided to remove external display support for now.

Current state:

- OLED Status Dashboard foundation is active.
- OLED Test proof-of-life screen remains active as diagnostics.
- U8g2 dependency is active for the dashboard and diagnostics screen.
- `tools/check_oled_test.py` ensures the proof screen stays on PaHub channel 1 and does not use direct OLED pins.
- `tools/check_oled_status_dashboard.py` ensures the dashboard helpers stay present, OLED remains on PaHub channel 1, ENV III remains on PaHub channel 0, and direct G8/G9 OLED wiring does not come back.
- Priority #7 planning started on 2026-08-15 in `docs/superpowers/plans/2026-08-15-external-display-revisit.md`.
- `tools/check_external_display_revisit.py` keeps the external-display safety notes and PaHub channel plan explicit.
- User chose the M5Stack Unit PaHub v2.1 on 2026-08-15.
- The PaHub uses a PCA9548AP I2C mux; project plan keeps the default address `0x70`.
- First firmware foundation detects the PaHub at `0x70`, selects ENV III on PaHub channel 0, and falls back to direct Grove if the hub is missing.
- SSD1309 OLED on PaHub channel 1.
- OLED Test probes `0x3C` and `0x3D`, now retrying at 400 kHz and then 100 kHz without tearing down the shared external I2C bus.
- OLED Test shows the active address, bus speed, and a short scan summary on the built-in LCD so `OLED not found` can distinguish channel/address misses from a missing PaHub.
- OK/Enter or `R` retries OLED detection.
- User confirmed OLED Test works great on Cardputer hardware on 2026-08-15.
- `src/oled_test.cpp` centralizes U8g2 drawing for both the OLED Status Dashboard and OLED Test diagnostics.
- `serviceOledStatusDashboard()` refreshes the OLED about once per second from `loop()`.
- `renderOledStatusDashboard()` builds five clipped lines for stable status views and compact 5x7 command/help sheets for selected Priority #8 feature screens.
- `setOledStatusLine(...)` exists so features can provide a short optional context line later without knowing U8g2 details.
- The dashboard shows current screen/mode, battery, Wi-Fi, MQTT when Pi Monitor is active or has been used, and feature context for Main Menu, Environment, Voice Memos, and Level.
- Priority #8 command/help pass added on 2026-09-13 for `WiFi Connect`, first-entry `Pi Monitor`, `SD Manager`, `RTC`, `GNSS Dash`, `GNSS Sky`, `Return Home`, `Breadcrumbs`, `LoRa Packets`, and `LoRa Diag`.
- Menu, Battery, System, WiFi Scan, Saved WiFi, Voice Memos, Environment, OLED Test, and Level were intentionally left unchanged for this pass.
- In Pi Monitor, the first-entry project list now shows a compact 5x7 command/help sheet; project command and Home / Diagnostics subviews keep the message-only 5x7 OLED view with one header and manually paged MQTT text.
- User confirmed the OLED Status Dashboard works across tested features on 2026-08-22.
- User confirmed a later OLED `not found` issue was resolved by replacing the Grove cable on 2026-09-05.
- Priority #7 is complete enough.
- User confirmed the Priority #8 command/help OLED pass checks out on hardware on 2026-09-13, and Priority #8 is stopped for now.

Future OLED guidance:

- Do not use G8/G9 directly for external I2C.
- Use the M5Stack Unit PaHub v2.1 path when ENV III and OLED need to stay connected together.
- ENV III on PaHub channel 0; SSD1309 OLED on PaHub channel 1.
- Consider a different display interface only after checking for microSD, upcoming LoRa/GNSS, keyboard, and internal hardware conflicts.
- Keep the OLED secondary: a glance/status display, not a duplicate of the built-in LCD.

## Upcoming Hardware Priorities

Priority #8: customize each feature for the external OLED.

- Keep OLED drawing centralized.
- Keep OLED on PaHub channel 1.
- Keep ENV III on PaHub channel 0.
- Make per-feature OLED content useful without duplicating the built-in LCD.
- Add guard coverage for feature-specific OLED helpers as the customization grows.
- 2026-09-13 command/help pass uses compact 5x7 OLED sheets for WiFi Connect, Pi Monitor project-list entry, SD Manager, RTC, GNSS Dash, GNSS Sky, Return Home, Breadcrumbs, LoRa Packets, and LoRa Diag.
- `GNSS Sky` intentionally omits the OLED banner and starts with command reminders before selected satellite data.
- Pi Monitor project command and diagnostics subviews keep the existing OLED MQTT message viewer.
- User confirmed this pass is working on hardware on 2026-09-13; stop with Priority #8 for now unless the user reopens OLED customization.

Priority #9: add DS3231 / AT24C32 I2C RTC module.

- User provided Amazon module link: `https://www.amazon.com/AT24C32-Replace-Arduino-Batteries-Included/dp/B07Q7NZTQS`
- Treat as a DS3231 RTC plus AT24C32 EEPROM module until hardware scan confirms details.
- First milestone is active as `RTC`.
- RTC is wired to PaHub channel 5: `I2C_HUB_RTC_CHANNEL = 5`.
- ENV III remains on PaHub channel 0 and SSD1309 OLED remains on PaHub channel 1.
- The screen selects PaHub channel 5 before each RTC probe/read.
- DS3231 is probed at `RTC_DS3231_ADDRESS = 0x68`.
- AT24C32 is probed at `RTC_AT24C32_ADDRESS = 0x57`.
- RTC date/time and temperature are read with raw Arduino `Wire` calls; no extra RTC library was added.
- `N` sets/corrects the DS3231 from NTP local time when Wi-Fi is already connected through WiFi Connect.
- RTC does not call `WiFi.begin`; it reuses the existing WiFi Connect flow and shows `Use WiFi Connect` if Wi-Fi is disconnected.
- `S` remains an offline fallback that sets the DS3231 from firmware build date/time plus current Cardputer uptime.
- NTP setting currently uses Mountain time through `RTC_TIMEZONE_POSIX = "MST7MDT,M3.2.0,M11.1.0"`.
- OK/Enter or `R` retries detection/read, and the built-in LCD read counter makes that retry visible.
- The screen shows `RTC online`, `RTC set NTP`, `RTC needs set`, `NTP sync failed`, `Use WiFi Connect`, `RTC time invalid`, `DS3231 not found`, or `PaHub ch5 missing`.
- The missing RTC path must be graceful: the feature screen remains usable and retries with OK/Enter or `R`.
- The AT24C32 EEPROM unused boundary is deliberate until there is a clear reason to store RTC-specific data.
- Priority #8 changes `RTC` OLED output to a compact 5x7 banner plus command/help sheet for NTP set, build-time set, retry, back, WiFi dependency, and PaHub channel.
- Guard: `tools/check_rtc_status.py`.
- Likely useful later for Environment log timestamps, Voice Memo file names, OLED clock/status, and Pi Monitor timestamps.
- Next RTC work should make timezone configurable if needed, then add selective consumers such as Environment logs, Voice Memo names, OLED clock/status, or Pi Monitor timestamps.

Priority #10: add M5Stack Cap LoRa-1262 for Cardputer Adv.

- User provided Amazon link: `https://www.amazon.com/dp/B0GWGXXKQT`
- Official M5Stack hardware includes SX1262 LoRa and ATGM336H GNSS.
- M5Stack docs say it connects through the Cardputer Adv EXT 2.54-14P interface.
- Important: the old NRF24/RF Scan path was removed from active firmware because it used the EXT SPI path needed by this hardware.
- First diagnostics milestone is active as `LoRa Diag`.
- RadioLib was chosen for SX1262 because the official M5Stack Arduino quick start uses RadioLib for this cap family.
- The screen detects PI4IOE5V6408 at `0x43`, sets `P0` high for the SX1262 antenna switch, starts SX1262 receive mode, and reports `LoRa not found` gracefully when the cap/radio is absent.
- RSSI displays live channel RSSI with `getRSSI(false)` before any packet is received; SNR is packet-only and displays `--pkt` until a matching LoRa packet arrives.
- The ATGM336H GNSS path is verified separately with `HardwareSerial` at `115200` 8N1 and byte/NMEA line counters.
- User confirmed on hardware on 2026-08-25 that cap detection, RF switch, radio init, live RSSI, and GNSS NMEA output are working.
- GNSS parser milestone uses the M5Stack TinyGPSPlus GitHub library, matching the official M5Stack Cap LoRa-1262 tutorial note.
- The parser displays fix/no-fix, satellites, HDOP, latitude, longitude, UTC time, NMEA sentence count, and checksum failures.
- User confirmed on hardware on 2026-08-26 that the GNSS parser is working.
- The GNSS UART/TinyGPSPlus parser now lives in `src/lora_gnss.cpp` so Priority #11 screens can reuse it without starting the SX1262 radio.
- `LORA_GNSS_FIX_STALE_MS = 5000` keeps an old location from looking current.
- No transmit behavior is enabled.
- Do not transmit until antenna, legal region/frequency, bandwidth/spreading plan, and TX power are deliberately set.

Cap LoRa-1262 documented pin map:

- `G5 NSS`
- `G4 IRQ`
- `G3 RST`
- `G6 BUSY`
- `G40 SCK`
- `G14 MOSI`
- `G39 MISO`
- `G15 GPS-TX` to Cardputer RX
- `G13 GPS-RX` from Cardputer TX
- `G8/G9` internal I2C for PI4IOE5V6408 and HY2.0-4P
- PI4IOE5V6408 `P0` controls the RF antenna switch

Shared conflict notes:

- SX1262 LoRa and microSD share SPI signal pins `G40/G39/G14`; chip selects are separate (`G5 NSS` for LoRa, `G12 CS` for microSD).
- `LoRa Diag` and `LoRa Messages` each own the SX1262 only while their screen is active and deselect microSD before radio init. `LoRa Diag` remains RX-only; `LoRa Messages` is the sole active Cardputer TX screen. The shared GNSS parser is also used by `GNSS Dash`, `GNSS Sky`, `Return Home`, and `Breadcrumbs`.
- SD-backed features call `prepareSharedSpiForSd()` so LoRa `NSS` is high and SPI is re-begun for microSD after LoRa has owned the bus.
- A small `sharedSpiOwner` flag prevents unnecessary SPI resets while a feature is already using the SD path, but forces a clean handoff after LoRa.
- Keep G8/G9 free for the internal Cardputer/cap I2C path; do not use them directly for OLED or ENV III.

Priority #11: Cap LoRa-1262 feature expansion.

- 2026-09-22: User confirmed bidirectional Cardputer/XIAO packet exchange and the Cardputer `LoRa Range` test. The XIAO's 60-second deep-sleep beacon test worked; node TX is now 5 dBm with DC-DC mode. Cardputer TX remains 2 dBm.
- Implemented design is in `docs/superpowers/plans/2026-09-22-lora-messages.md`: the packet monitor and cap/GNSS diagnostics now share one paged RX-only screen; typed messaging replaces the range UI. Old `/tracks/lora-rangeNNN.csv` files are historical data and must not be deleted.
- The XIAO's successful minute deep-sleep experiment is disabled for the current awake serial messaging peer. Message/ACK hardware validation remains pending; its display belongs to a future chat.

- First milestone added: `GNSS Dash`.
- `GNSS Dash` starts only the ATGM336H GNSS serial parser and does not initialize SX1262, claim the shared SPI bus, or transmit.
- Built-in LCD shows the GNSS Dashboard view: fix/no-fix, satellites, HDOP, latitude, longitude, speed, altitude, UTC/date, fix age, NMEA line count, checksum count, and byte count.
- Priority #8 changes `GNSS Dash` OLED output to a compact 5x7 banner plus command/help sheet.
- OK/Enter or `R` restarts the parser; Backspace returns to the main menu and stops the GNSS serial object.
- No transmit behavior is enabled.
- User reported `GNSS Dash` is looking good on hardware on 2026-08-26.
- Guard: `tools/check_lora_gnss_dashboard.py`.
- Second milestone added: `GNSS Sky`, the GNSS Satellite Sky View.
- `GNSS Sky` starts only the ATGM336H GNSS serial parser and does not initialize SX1262, claim the shared SPI bus, or transmit.
- `src/lora_gnss.cpp` now parses `$GxGSV` satellite-in-view NMEA sentences for constellation/PRN, elevation, azimuth, and SNR.
- `GNSS Sky` draws horizon/elevation rings, cardinal direction labels, and SNR-colored satellite dots that move as GSV updates arrive.
- The screen reports plotted satellites, GSV satellites-in-view, GSV sentence count/age, fix status, HDOP, UTC, and strongest SNR satellite.
- Satellite records expire after `GNSS_SKY_STALE_MS = 15000` so stale sky positions do not look live.
- Arrow keys cycle the selected satellite through active plotted satellites, skip inactive/stale entries, and clamp/reset when the active list changes.
- The built-in LCD highlights the selected satellite dot while the right-side panel stays focused on overall sky/fix summary.
- Priority #8 removes the `GNSS Sky` OLED banner and shows command reminders plus selected satellite details: label, constellation, PRN/satellite ID, SNR, elevation, azimuth, compass direction, and last-seen age.
- OK/Enter or `R` restarts the parser; Backspace returns to the main menu and stops the GNSS serial object.
- No transmit behavior is enabled.
- User reported the base `GNSS Sky` screen is working great on hardware on 2026-08-28.
- User confirmed the selected-satellite update is working on hardware on 2026-08-28.
- Guard: `tools/check_lora_gnss_sky_view.py`.
- Third milestone added: `Return Home`, the first Waypoint / Return Home pass.
- `Return Home` starts only the ATGM336H GNSS serial parser and does not initialize SX1262, claim the shared SPI bus, or transmit.
- `S` saves or updates one saved home point from the current fresh GNSS fix into Preferences/NVS namespace `scoober_home`.
- `D` clears the saved home point from Preferences/NVS.
- The screen shows distance and bearing from the current fresh GNSS fix back to the saved home point, plus a compass direction and arrival status inside `RETURN_HOME_ARRIVAL_RADIUS_METERS = 10.0f`.
- Priority #8 changes `Return Home` OLED output to a compact 5x7 banner, description, command/help sheet, distance, and bearing.
- OK/Enter or `R` restarts the shared GNSS parser without deleting the saved home point.
- No transmit behavior is enabled.
- Guard: `tools/check_return_home.py`.
- User confirmed `Return Home` is working great on hardware on 2026-08-29.
- Fourth milestone added: `Breadcrumbs`, the Breadcrumb Logger first pass.
- `Breadcrumbs` starts only the ATGM336H GNSS serial parser and does not initialize SX1262, claim the shared SPI bus for LoRa, or transmit.
- `S` starts/stops CSV logging to microSD under `/tracks/trackNNN.csv`.
- Each log writes `BREADCRUMB_LOG_HEADER`, then appends fresh GNSS fixes every `BREADCRUMB_LOG_SAMPLE_INTERVAL_MS = 5000`.
- Breadcrumb rows contain uptime seconds, UTC, date, latitude, longitude, satellites, HDOP, speed, and altitude.
- Stale/no-fix samples are skipped and counted as missed fixes instead of writing bad coordinates.
- Files are flushed after each row and closed when leaving the screen, pressing OK/Enter or `R`, or stopping the log.
- Priority #8 changes `Breadcrumbs` OLED output to a compact 5x7 banner, GNSS CSV description, command/help sheet, point/missed counts, and file hint.
- No transmit behavior is enabled.
- Guard: `tools/check_breadcrumb_logger.py`.
- Historical fifth milestone (now merged into `LoRa Diag`): `LoRa Packets`, the LoRa Packet Monitor first pass.
- `LoRa Packets` initializes SX1262 with the same known-good RX settings as `LoRa Diag`.
- It detects the PI4IOE5V6408 cap expander and drives P0 high for the RF switch.
- It is an RX-only packet viewer for matching LoRa settings and has no transmit action.
- Built-in LCD shows packet count, CRC mismatch count, receive error count, payload preview, payload length, age, RSSI, SNR, frequency, spreading factor, and bandwidth.
- Before a packet is decoded, RSSI is shown as channel/noise RSSI; packet count, packet SNR, payload length, age, and payload preview stay in the no-packet state until matching LoRa frames arrive.
- Payload previews are sanitized/clipped to `LORA_PACKET_MONITOR_PAYLOAD_MAX_CHARS = 64`.
- `C` clears packet counters and payload preview; OK/Enter or `R` restarts the RX-only monitor.
- Priority #8 changes `LoRa Packets` OLED output to a compact 5x7 banner, command/help sheet, matching-sender hint, packet/error count summary, and no-TX reminder.
- No transmit behavior is enabled.
- Current guard: `tools/check_lora_messages.py`.
- User reported on 2026-09-09 that `Breadcrumbs` works fine on hardware.
- User reported on 2026-09-09 that `LoRa Packets` shows idle channel/noise RSSI while `Pk`, CRC, Err, SNR, Len, Age, and payload remain unchanged; this is expected until a matching LoRa packet is decoded.
- User confirmed on 2026-09-13 that the clarified idle/listening `LoRa Packets` behavior is working fine on hardware.
- LoRa TX planning started on 2026-09-13 in `docs/superpowers/plans/2026-09-13-lora-tx-planning.md`.
- `LoRa Range Test is the only Cardputer TX feature`; it stays a manual, armed ping/ACK diagnostic rather than a general transmit path.
- Planning assumes US 902-928 MHz for the current device location and keeps `LoRa Diag` plus `LoRa Packets` RX-only/no-transmit.
- First planned TX settings match the RX monitors: 915.0 MHz, 125 kHz, SF12, CR 4/5, sync word 0x34, preamble 20.
- First planned TX power is 2 dBm, with any increase treated as a separate deliberate choice after antenna, region, and rule-path review.
- Antenna must be installed before any TX test; start with the included Cap LoRa-1262 SMA antenna.
- `LoRa Ping / Range Test` is implemented as `LoRa Range`: `A` creates `/tracks/lora-rangeNNN.csv` and arms the test, while `P` sends `SCBR,PING,1,scoober-cardputer,<seq>,<uptime_ms>,<battery_pct>` only when armed.
- Each manual send has a 10 seconds cooldown and opens a 10 seconds ACK window for `SCBR,ACK,1,xiao-sx1262-ack,<seq>,<remote_rssi>,<remote_snr>,<remote_uptime_ms>`.
- It logs ACKs, ACK timeouts, and TX errors with local/remote RSSI/SNR, GNSS or RTC timestamp data when available, and battery percentage. `C` clears display counters, OK/Enter or `R` reinitializes the radio, and Backspace sleeps it.
- The OLED uses the compact command sheet with arm, ping, clear, restart, radio-sleep, counters, and latest ACK metrics.
- Current guard: `tools/check_lora_messages.py`.
- Added a separate XIAO ESP32-S3 Wio-SX1262 LoRa ACK Node scaffold on 2026-09-13 in `nodes/xiao_sx1262_lora_ack`.
- XIAO node defaults target the standalone Wio-SX1262 for XIAO header pinout: `GPIO5` NSS, `GPIO2` DIO1/IRQ, `GPIO3` RST, `GPIO4` BUSY, `GPIO1` RF switch, and SPI `GPIO7/GPIO8/GPIO9`. The separate B2B kit uses the optional `xiao_esp32s3_b2b` environment with `GPIO41/GPIO39/GPIO42/GPIO40/GPIO38`.
- XIAO node uses a 3.0 V TCXO setting, `radio.setDio2AsRfSwitch(true)`, receive-mode RF switch high, transmit-mode RF switch low, and the same 915.0 MHz / 125 kHz / SF12 / CR 4/5 / sync word 0x34 / preamble 20 / 2 dBm radio settings as the Cardputer RX screens.
- XIAO node sends `SCBR,NODE,1,xiao-sx1262-ack` only from serial `p` or the user button, replies to `SCBR,PING,1` with `SCBR,ACK,1,xiao-sx1262-ack`, and has no periodic beacons.
- The standalone XIAO node hardware path was confirmed on 2026-09-18: manual `P` probes were received by Cardputer `LoRa Packets`. The Cardputer-side range-test ACK and CSV flow remains the next dedicated hardware check.
- Guard: `tools/check_xiao_sx1262_node.py`.

## ESP-NOW And RC Notes

Original future idea included:

- ESP-NOW RC controller
- W/S throttle
- A/D steering

User later decided:

- This Cardputer project is not going to be the RC controller.
- ESP-NOW RC placeholder was removed.

Current state:

- No active ESP-NOW code.
- Do not add RC car control unless explicitly asked.

## Raspberry Pi Command Center Notes

Priority #6 planning started on 2026-07-25.
The first firmware pass was added on 2026-07-27.
User confirmed Pi Monitor MQTT viewing works on Cardputer hardware on 2026-07-29.
The first whitelisted `read_now` command publisher was added on 2026-07-30.
User confirmed Pi Monitor `read_now` command publishing works on Cardputer hardware on 2026-07-30.
User confirmed Cardputer MQTT status/availability publishing works on hardware on 2026-08-15.
The fixed-choice `set_interval` command publisher was added on 2026-08-15.
User confirmed Pi Monitor `set_interval` command publishing works on Cardputer hardware on 2026-08-15.
Target selection was added on 2026-08-15.
User confirmed Pi Monitor target selection works on Cardputer hardware on 2026-08-15.

Decision:

- Use Wi-Fi MQTT as the first Raspberry Pi command center transport.
- Start with a read-only `Pi Monitor` screen, then add command publishing one whitelisted JSON action at a time.
- Reuse WiFi Connect for network access; do not create a second Wi-Fi credential path.
- Read Pi/MQTT settings from microSD `/config/pi.txt`.
- Keep direct shell control, remote command execution, and Pi admin actions out of the first milestone.

Why MQTT:

- The Raspberry Pi learning repo already has an MQTT broker/listener architecture.
- The Pi listener subscribes to `home/#`.
- The ESP32-C3 test node already publishes under `home/devices/esp32-c3-test/...`.
- MQTT lets the Cardputer watch device status without requiring a new Pi HTTP API.
- Later command features can publish safe JSON commands to existing `commands` topics.

`/config/pi.txt` format:

```text
mqtt_host=10.0.0.180
mqtt_port=1883
device_id=scoober-cardputer
command_target=esp32-c3-test
project=esp32-c3-test|ESP32-C3 Test|basic
```

`command_target` remains compatible as the initial/default command target. Add one `project=id|Label|profile` line per MQTT project; the first firmware profile is `basic`, with `read_now` and fixed-choice `set_interval`.

Current Pi Monitor firmware behavior:

- Menu item: `Pi Monitor`.
- Requires Wi-Fi to already be connected through WiFi Connect.
- Uses `PubSubClient` for MQTT and `ArduinoJson` for small status/telemetry summaries.
- Connects to the configured MQTT broker.
- Subscribes to `home/#` for read-only visibility while testing.
- Publishes retained Cardputer availability to `home/devices/scoober-cardputer/availability`.
- Publishes retained Cardputer status JSON to `home/devices/scoober-cardputer/status` after MQTT connect and about once per minute while Pi Monitor is open.
- Uses an MQTT last-will so unexpected disconnects can mark Cardputer availability as `offline`.
- User confirmed status/availability publishing works on Cardputer hardware on 2026-08-15.
- Opens on an LCD project list with `Home / Diagnostics`, configured projects, and discovered device IDs.
- Selecting a project opens an LCD command list for that project's command profile.
- Home / Diagnostics keeps broker/device diagnostics on the LCD and shows one header plus manually paged all-message MQTT text on the OLED.
- A selected project keeps command selection on the LCD and shows one header plus manually paged project-filtered MQTT text on the OLED.
- Shows MQTT connection status, broker, message count, last topic/payload, command response display, and compact project/device status.
- Treats `home/devices/<device>/<kind>` topics as structured device-list updates.
- Shows `home/devices/<device>/responses`, nested response topics, or post-command updates from `command_target` on a dedicated `Resp:` line.
- User reported on 2026-08-06 that `Resp:` still stayed on `waiting`; use `Last` / `Pay` as the reliable command-feedback view unless explicit acknowledgments become important.
- Stores up to `MAX_PI_MONITOR_DEVICES = 6` devices, up to `MAX_PI_MONITOR_PROJECTS = 8` configured projects, and up to `MAX_PI_MONITOR_MESSAGES = 8` recent MQTT messages for OLED filtering. MQTT message history keeps longer topic/payload text for the Pi Monitor OLED viewer.
- Fails gracefully when Wi-Fi, SD config, or broker connection is missing.
- Backspace returns from a Pi Monitor subview to the project list, then returns to the menu and stops the MQTT connection.
- OK opens the highlighted project or sends the highlighted command; in Home / Diagnostics, OK retries MQTT connection.
- `C` publishes `{"command":"read_now"}` to `home/devices/<command_target>/commands`.
- `T` cycles the command target through `command_target` and discovered device IDs, excluding the Cardputer's own MQTT identity.
- `I` cycles fixed `set_interval` choices: 10, 30, 60, and 300 seconds.
- `S` advances the OLED MQTT message page.
- User confirmed the project/command UI and manual `S` OLED MQTT message paging work on Cardputer hardware on 2026-09-05.
- `R` clears the device list and reconnects.
- `D` disconnects MQTT.
- Does not publish arbitrary command text, shell commands, or Raspberry Pi admin actions.

Remaining Priority #6 work:

- Treat Priority #6 as complete enough unless the Pi-side listener needs new command support.
- Revisit `Resp:` only if future commands need explicit success/failure acknowledgments.
- Consider MQTT authentication after live Mosquitto configuration is confirmed on the Raspberry Pi.

## Build And Verification Notes

Known good build command:

```sh
python -m platformio run
```

Known good upload command:

```sh
python -m platformio run --target upload
```

The project may also work with `pio run`, but Codex previously saw `pio` missing from PATH.

Run all guards:

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
python tools/check_lora_messages.py
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

Known build warning:

- M5Unit-ENV can emit an ambiguity warning in its own `I2C_Class.cpp` around `Wire.requestFrom`.
- The build has still succeeded with this warning.

## Documentation Maintenance Notes

When adding a feature:

1. Add or update a guard script in `tools/`.
2. Run the guard and confirm it fails before implementation.
3. Implement the feature.
4. Update README and this notes file.
5. Run all guards.
6. Run `python -m platformio run`.

Keep `todo.md` current after each milestone.
