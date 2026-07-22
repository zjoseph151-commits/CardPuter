from pathlib import Path

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


def test_credentials_are_not_in_firmware_yet():
    forbidden_source_tokens = [
        "WiFi.begin",
        "password",
        "WIFI_PASSWORD",
        "WIFI_PASS",
        "wifiPassword",
        "wifiPass",
    ]

    source_lower = SOURCE.lower()
    for token in forbidden_source_tokens:
        haystack = source_lower if token == "password" else SOURCE
        assert token not in haystack, (
            "Wi-Fi credentials/connect code should not be in firmware until the "
            f"intentional connect milestone: {token}"
        )


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
    test_credentials_are_not_in_firmware_yet()
    test_sd_credential_strategy_is_documented()
    test_local_config_paths_are_ignored()
    print("Wi-Fi credential strategy checks passed.")
