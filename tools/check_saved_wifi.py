from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "src" / "main.cpp"
README = ROOT / "README.md"

main = MAIN.read_text(encoding="utf-8")
readme = README.read_text(encoding="utf-8")

required_main_tokens = [
    "#include <Preferences.h>",
    "Screen::WifiSaveConfirm",
    "Screen::WifiSaveResult",
    "Screen::SavedWifi",
    "Screen::SavedWifiDeleteConfirm",
    "Screen::SavedWifiDeleteResult",
    '"Saved WiFi"',
    "Preferences wifiPrefs;",
    "loadSavedWifiNames()",
    "saveWifiName(",
    "deleteSavedWifiName(",
    "renderSavedWifiList()",
    "renderWifiSaveConfirm()",
    "renderWifiSaveResult()",
    "renderSavedWifiDeleteConfirm()",
    "renderSavedWifiDeleteResult()",
    "MAX_SAVED_WIFI_NAMES",
    '"Network saved"',
    '"Network deleted"',
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
    "D to delete",
]

for token in required_readme_tokens:
    assert token in readme, f"Missing README saved WiFi note: {token}"

print("Saved WiFi checks passed.")
