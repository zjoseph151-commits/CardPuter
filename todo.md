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
- External display support now has an OLED Status Dashboard foundation plus OLED Test diagnostics.
- Do not use G8/G9 directly for external I2C.
- Planning doc: `docs/superpowers/plans/2026-08-15-external-display-revisit.md`.
- Guard: `tools/check_external_display_revisit.py`.
- Guard: `tools/check_oled_status_dashboard.py`.
- User chose the M5Stack Unit PaHub v2.1 on 2026-08-15.
- Chosen hardware path:
  - Cardputer Grove G2/G1 to Unit PaHub v2.1 input
  - default PaHub address `0x70`
  - ENV III on PaHub channel 0
  - SSD1309 OLED on PaHub channel 1
- First firmware foundation added:
  - detect PaHub at `0x70`
  - select ENV III channel 0 before Environment init/read
  - fall back to direct Grove when no PaHub is present
- OLED Test proof-of-life added:
  - U8g2 dependency active
  - select PaHub channel 1 before OLED probe/init/draw
  - probe `0x3C` and `0x3D`
  - draw a simple SSD1309 proof pattern
  - OK/Enter or `R` retries OLED detection
- User confirmed OLED Test works on hardware on 2026-08-15.
- OLED Status Dashboard foundation added on 2026-08-16:
  - centralize status drawing in `renderOledStatusDashboard()`
  - refresh from `loop()` through `serviceOledStatusDashboard()`
  - keep optional feature context available through `setOledStatusLine(...)`
  - show current screen/mode, battery, Wi-Fi, MQTT when Pi Monitor is active or has been used, and compact feature context
  - Main Menu shows Scoober, battery, Wi-Fi, mode/menu selection
  - Pi Monitor shows MQTT, target, command, and message/device context
  - Environment shows temperature, humidity, and valid pressure or `Press: invalid`
  - Voice Memos shows recording timer or selected memo/status
  - RF Scan shows quiet channel and selected channel context
- Hardware test checklist:
  - ENV III through Unit PaHub v2.1 channel 0: user confirmed mostly working on 2026-08-15
  - keyboard navigation with Unit PaHub v2.1 attached: user confirmed working on 2026-08-15
  - direct Grove ENV III fallback after this change: worth spot-checking
  - OLED Test through Unit PaHub v2.1 channel 1: user confirmed working on 2026-08-15
  - OLED Status Dashboard across Main Menu, Pi Monitor, Environment, Voice Memos, and RF Scan: user confirmed everything is working on 2026-08-22
  - Environment after dashboard updates, with both modules attached: user confirmed working on 2026-08-22
  - keyboard navigation while dashboard is refreshing: user confirmed working on 2026-08-22
- Priority #7 is complete enough. Next OLED work belongs under Priority #8.

## Priority 8: Customize Each Feature For The External OLED

- Goal: make the SSD1309 OLED more useful per feature without turning it into a duplicate of the built-in LCD.
- Keep the built-in Cardputer LCD as the main control screen.
- Keep OLED drawing centralized; avoid exposing U8g2 details throughout feature modules.
- Keep using PaHub channel 1 for OLED.
- Keep ENV III on PaHub channel 0.
- Keep graceful behavior if PaHub or OLED is missing.
- First milestone ideas:
  - Main Menu: show project identity, battery, Wi-Fi/MQTT summary, and selected feature.
  - Battery/System: show glanceable power/system values.
  - WiFi screens: show connection state, selected SSID, IP/status, and saved count.
  - Pi Monitor: show MQTT status, selected target, selected command, and compact activity.
  - Voice Memos: show recording timer, playback/recording state, file name, and storage status.
  - Environment: show temperature, humidity, pressure validity, logging state, and maybe trend later.
  - RF Scan: show selected channel, quiet channel recommendations, and scan state.
  - Level: show compact X/Y or level/tilt status.
- Add or update guards so feature-specific OLED helpers stay present and direct G8/G9 OLED wiring does not come back.
- Run all guard scripts and `python -m platformio run` after each OLED customization pass.

## Priority 9: Add RTC Module

- Hardware requested: DS3231 / AT24C32 I2C RTC module from Amazon:
  - `https://www.amazon.com/AT24C32-Replace-Arduino-Batteries-Included/dp/B07Q7NZTQS`
- Purpose:
  - provide reliable date/time without depending on Wi-Fi
  - improve Environment CSV timestamps
  - improve Voice Memo file names
  - support future OLED clock/status display
  - support Pi Monitor timestamps when useful
- First milestone should be a safe RTC foundation, not a broad rewrite:
  - review current PaHub/I2C implementation before coding
  - choose and document a PaHub channel for RTC; do not use ENV channel 0 or OLED channel 1
  - detect DS3231 gracefully and keep firmware working when missing
  - read and display current time on a simple diagnostics/status path
  - add an optional time-set path later, likely from compile time, serial, Wi-Fi/NTP, or a config file
  - keep AT24C32 EEPROM unused unless there is a clear reason
- Expected I2C notes to verify before coding:
  - DS3231 RTC commonly uses address `0x68`
  - AT24C32 EEPROM commonly uses address `0x57`
  - exact module behavior should be confirmed with an I2C scan or library docs
- Guard requirements:
  - RTC must not replace ENV/OLED PaHub channel assignments
  - missing RTC must be graceful
  - PlatformIO build must pass

## Priority 10: Add Cap LoRa 1262

- Hardware requested: M5Stack Cap LoRa-1262 for Cardputer Adv from Amazon:
  - `https://www.amazon.com/dp/B0GWGXXKQT`
- Known hardware direction:
  - official M5Stack Cardputer Adv cap
  - SX1262 LoRa radio
  - ATGM336H GNSS
  - designed for the Cardputer Adv EXT 2.54-14P interface
  - supports the 868-923 MHz LoRa band according to product/docs
- Important conflict note:
  - the Cap LoRa-1262 uses the Cardputer Adv EXT interface
  - current NRF24 RF Scan also uses EXT SPI-related pins
  - do not assume NRF24 RF Scan hardware and Cap LoRa-1262 can be used at the same time
  - document exact pin usage and decide whether LoRa temporarily replaces NRF24 for testing
- First milestone should be diagnostics only:
  - review M5Stack docs and pin map before coding
  - choose a LoRa library compatible with SX1262 and ESP32-S3
  - add a graceful "LoRa not found" diagnostics screen or status section
  - verify GNSS serial path separately if used
  - do not transmit until antenna, region/frequency, and TX power are deliberately set
  - avoid interfering with Wi-Fi, microSD, OLED, ENV III, and keyboard behavior
- Guard requirements:
  - preserve current NRF24 RF Scan code unless the user explicitly chooses to replace it
  - document any shared EXT pin conflicts
  - PlatformIO build must pass

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
- Environment CSV columns: uptime seconds, temperature C, temperature F, humidity percent, pressure hPa.
- Added Environment log naming before start, `L` start/stop logging control, log file/sample display, and graceful SD-missing behavior.
- Added Environment pressure sanity filtering after PaHub hardware testing showed impossible QMP6988 pressure and `nan` altitude; altitude is no longer displayed or logged.
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
