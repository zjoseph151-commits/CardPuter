from pathlib import Path
import re

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
README = (ROOT / "README.md").read_text(encoding="utf-8")
NOTES = (ROOT / "notes.md").read_text(encoding="utf-8")
TODO = (ROOT / "todo.md").read_text(encoding="utf-8")
GITIGNORE = (ROOT / ".gitignore").read_text(encoding="utf-8")
SOURCE = firmware_source_text()


def require_tokens(text, tokens, label):
    for token in tokens:
        assert token in text, f"Missing {label}: {token}"


def test_connect_flow_uses_sd_config():
    required_source_tokens = [
        'WIFI_CONFIG_PATH = "/config/wifi.txt"',
        "WIFI_CONNECT_TIMEOUT_MS = 15000",
        "Screen::WifiConnect",
        '"WiFi Connect"',
        "showWifiConnect()",
        "renderWifiConnect()",
        "initWifiConfigSd()",
        "readWifiCredentialsFromSd(",
        "connectWifiFromConfig()",
        "disconnectWifi()",
        "wifiStatusText(",
        'key == "ssid"',
        'key == "password"',
        "WiFi.begin(wifiConnectSsid.c_str(), wifiPassword.c_str())",
        "WL_CONNECTED",
        "WiFi.localIP().toString()",
        "WIFI_CONNECT_TIMEOUT_MS",
        "OK retry",
        "D disconnect",
        "Back menu",
        "No wifi config.",
        "Missing SSID.",
    ]

    require_tokens(SOURCE, required_source_tokens, "Wi-Fi connect firmware")


def test_credentials_are_not_hardcoded():
    forbidden_source_tokens = [
        "WIFI_PASSWORD",
        "WIFI_PASS",
        "wifiPass =",
        "password=YourNetworkPassword",
        "YourNetworkPassword",
    ]

    for token in forbidden_source_tokens:
        assert token not in SOURCE, f"Do not hardcode Wi-Fi credentials: {token}"

    assert not re.search(r"WiFi\.begin\s*\(\s*\"", SOURCE), (
        "WiFi.begin must use SD-loaded variables, not string literals"
    )
    assert 'wifiPrefs.putString("password"' not in SOURCE
    assert 'wifiPrefs.putString("pass"' not in SOURCE


def test_sd_credential_strategy_is_documented():
    strategy_tokens = [
        "/config/wifi.txt",
        "ssid=YourNetworkName",
        "password=YourNetworkPassword",
        "Do not hardcode credentials",
        "Do not store Wi-Fi passwords in Preferences/NVS",
        "WiFi.begin",
        "microSD",
    ]

    require_tokens(README, strategy_tokens, "README Wi-Fi credential strategy")
    require_tokens(NOTES, strategy_tokens, "notes Wi-Fi credential strategy")
    require_tokens(TODO, strategy_tokens, "TODO Wi-Fi credential strategy")


def test_local_config_paths_are_ignored():
    require_tokens(
        GITIGNORE,
        [
            "/config/wifi.txt",
            "/wifi.txt",
        ],
        ".gitignore Wi-Fi credential ignore",
    )


if __name__ == "__main__":
    test_connect_flow_uses_sd_config()
    test_credentials_are_not_hardcoded()
    test_sd_credential_strategy_is_documented()
    test_local_config_paths_are_ignored()
    print("Wi-Fi credential strategy checks passed.")
