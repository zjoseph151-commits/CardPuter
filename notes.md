# Scoober Technical Notes

These notes preserve project context for future Codex sessions. They are intentionally detailed and include decisions, experiments, and hardware lessons that are easy to lose in chat history.

## Project Identity

- Firmware/project name shown to the user: `Scoober`
- Original scaffold idea: `cardputer-project-launcher`
- Current firmware version constant: `v0.1.0`
- Main source file: `src/main.cpp`
- Target board in PlatformIO: `m5stack-stamps3`
- Framework: Arduino
- Device: M5Stack Cardputer Adv Version

## Current Menu State

The active menu is defined in `MENU_ITEMS`:

```cpp
{"Battery", Screen::BatteryInfo}
{"System", Screen::SystemInfo}
{"WiFi Scan", Screen::WifiScan}
{"Saved WiFi", Screen::SavedWifi}
{"Voice Memos", Screen::VoiceMemos}
{"Environment", Screen::Environment}
{"Level", Screen::LevelTool}
```

Removed menu items:

- Keyboard Test
- Display Test
- SD Card Test
- ESP-NOW RC Placeholder
- OLED Test
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
- Voice Memos: `R` records/stops, OK plays, `D` deletes after confirmation

Do not re-add the old footer text inside every feature. The user asked to remove it.

## Display Refresh Notes

Earlier dynamic screens flickered because the whole screen/header was redrawn on every update.

Current pattern:

- Call `drawScreenFrame(title)` when entering a screen.
- For dynamic content, call `beginContentDraw()`, draw to `contentCanvas`, then `commitContentDraw()`.
- `commitContentDraw()` pushes the sprite to `(0, CONTENT_TOP)` so the header remains stable.

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

Wi-Fi is scan-only.

Current rules:

- No credentials.
- No calls to `WiFi.begin`.
- No connection attempts.
- Saved WiFi stores only SSID names.
- Saved names survive reboot through Preferences/NVS.

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

## Voice Memo Notes

Voice memo feature uses the built-in mic/speaker and microSD.

Storage:

- Directory: `/memos`
- File names: `memo001.wav` through `memo999.wav`
- Format: WAV, PCM, 16-bit, mono
- Sample rate: 16000 Hz
- Max recording time: 30 seconds

Pins:

- SD SCK: G40
- SD MISO: G39
- SD MOSI: G14
- SD CS: G12

Implementation notes:

- Recording writes a placeholder WAV header first and patches it when recording stops.
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
- Uses QMP6988 for pressure/altitude

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
- Press `L` on the Environment screen to open a log-name entry screen before starting optional CSV logging.
- OK/Enter starts logging with the typed name; Backspace deletes characters, or cancels when the name is blank.
- Names allow letters, numbers, spaces, `_`, and `-`; spaces are saved as underscores and names are capped at 16 characters.
- Blank names fall back to `env001.csv`, `env002.csv`, etc.
- Named sessions create files in `/env`, such as `backyard001.csv`.
- CSV header: `uptime_s,temp_c,temp_f,humidity_pct,pressure_hpa,altitude_m`.
- The screen shows the active log file name and sample count.
- Backspace stops any active Environment log before returning to the main menu.
- Missing SD card or `/env` creation failure shows a status message and leaves the firmware usable.

Important history:

- A `TwoWire envWire(1)` experiment compiled, but on hardware QMP6988 pressure read as `0.0 hPa` and altitude as `inf m`.
- Reverted ENV III to the global `Wire` path because it reads both SHT30 and QMP6988 correctly.

Manual wiring if not using Grove connector:

- VCC -> Grove 5V
- GND -> GND
- SDA -> Grove SDA
- SCL -> Grove SCL

Guard:

- `tools/check_environment_feature.py`

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

The user bought/planned a 2.42 inch SSD1309 OLED, then decided not to add it right now.

What was tried:

- Added U8g2 dependency and OLED Test screen.
- Tried using G8/G9 to preserve Grove.
- Arrow keys stopped working.
- Investigation showed G8/G9 are Cardputer Adv internal I2C lines, used by onboard hardware including keyboard path.
- Moved OLED to Grove G2/G1; test worked.
- User decided to remove external display support for now.

Current state:

- No active OLED code.
- No U8g2 dependency.
- README keeps a future idea note.
- `tools/check_oled_test.py` ensures the inactive OLED feature does not return accidentally.

Future OLED guidance:

- Do not use G8/G9 directly for external I2C.
- Consider I2C mux/buffer/expander or a different display interface if Grove must remain available.
- Reintroduce display support only when there is a deliberate pin plan.

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

This remains a future direction but no networking code exists yet.

Likely future concerns:

- Secure Wi-Fi credential handling
- Command protocol
- UI structure for commands/status
- Whether the Pi talks over Wi-Fi, BLE, USB serial, MQTT, HTTP, or another channel
- How to preserve offline/local utility features while adding networking

Do not add this until the user asks.

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
python tools/check_display_refresh.py
python tools/check_environment_feature.py
python tools/check_level_tool.py
python tools/check_menu_structure.py
python tools/check_oled_test.py
python tools/check_power_status.py
python tools/check_saved_wifi.py
python tools/check_voice_memos.py
python tools/check_wifi_scroll.py
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
