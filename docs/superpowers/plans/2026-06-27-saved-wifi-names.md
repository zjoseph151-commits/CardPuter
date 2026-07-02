# Saved WiFi Names Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let Scoober save selected WiFi SSID names persistently and browse them from a separate main menu feature.

**Architecture:** Use ESP32 Preferences/NVS as the persistence layer, keeping only SSID names and never storing passwords or attempting connections. Extend the existing single-file state machine with two new screens: a save confirmation screen reached from WiFi Scan, and a Saved WiFi list screen reached from the main menu.

**Tech Stack:** PlatformIO, Arduino framework, M5Cardputer, ESP32 `Preferences`, Python guard scripts.

---

### Task 1: Guard Test

**Files:**
- Create: `tools/check_saved_wifi.py`

- [ ] **Step 1: Write the failing guard test**

```python
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "src" / "main.cpp"
README = ROOT / "README.md"

main = MAIN.read_text(encoding="utf-8")
readme = README.read_text(encoding="utf-8")

required_main_tokens = [
    "#include <Preferences.h>",
    "Screen::WifiSaveConfirm",
    "Screen::SavedWifi",
    '"Saved WiFi"',
    "Preferences wifiPrefs;",
    "loadSavedWifiNames()",
    "saveWifiName(",
    "renderSavedWifiList()",
    "renderWifiSaveConfirm()",
    "MAX_SAVED_WIFI_NAMES",
]

for token in required_main_tokens:
    assert token in main, f"Missing expected saved WiFi token: {token}"

assert "password" not in main.lower(), "Saved WiFi feature must not store passwords"
assert "WiFi.begin" not in main, "Saved WiFi feature must not connect to WiFi"

required_readme_tokens = [
    "Saved WiFi",
    "only SSID names",
    "OK/Enter to save",
    "R to rescan",
]

for token in required_readme_tokens:
    assert token in readme, f"Missing README saved WiFi note: {token}"

print("Saved WiFi checks passed.")
```

- [ ] **Step 2: Run the guard test to verify it fails**

Run: `python tools/check_saved_wifi.py`

Expected: FAIL because saved WiFi storage and screens are not implemented yet.

### Task 2: Firmware Implementation

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Add Preferences storage and new screens**

Add `#include <Preferences.h>`, a `WifiSaveConfirm` screen, a `SavedWifi` screen, saved-name arrays, and helpers:

```cpp
constexpr int MAX_SAVED_WIFI_NAMES = 20;
constexpr int SAVED_WIFI_VISIBLE_ROWS = 5;

Preferences wifiPrefs;
String savedWifiNames[MAX_SAVED_WIFI_NAMES];
int savedWifiCount = 0;
int selectedSavedWifiIndex = 0;
int savedWifiScrollOffset = 0;
```

- [ ] **Step 2: Load saved names on boot**

Call `loadSavedWifiNames()` during `setup()` before drawing the main menu.

- [ ] **Step 3: Save selected WiFi SSIDs**

Change OK/Enter in WiFi Scan to open a confirmation screen. In confirmation, OK/Enter saves the selected SSID, Backspace cancels to WiFi Scan.

- [ ] **Step 4: Preserve rescan behavior**

Move WiFi rescan from OK/Enter to `R`/`r`.

- [ ] **Step 5: Add Saved WiFi list**

Add `Saved WiFi` to the main menu and render saved SSID names with arrow-key scrolling.

### Task 3: Documentation and Verification

**Files:**
- Modify: `README.md`
- Run: `python tools/check_saved_wifi.py`
- Run: `python tools/check_menu_structure.py`
- Run: `python tools/check_wifi_scroll.py`
- Run: `pio run`

- [ ] **Step 1: Update README**

Document the new flow: WiFi Scan uses OK/Enter to save, `R` to rescan, and Saved WiFi lists only SSID names.

- [ ] **Step 2: Verify guards and build**

Expected: all guard scripts pass and PlatformIO build succeeds.
