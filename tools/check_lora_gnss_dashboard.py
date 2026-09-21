from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
SOURCE = firmware_source_text()
RX_ONLY_SOURCE = firmware_source_text({"lora_range_test.cpp"})
README = (ROOT / "README.md").read_text(encoding="utf-8")
NOTES = (ROOT / "notes.md").read_text(encoding="utf-8")
TODO = (ROOT / "todo.md").read_text(encoding="utf-8")


def require_tokens(text, tokens, label):
    for token in tokens:
        assert token in text, f"Missing {label}: {token}"


def assert_tokens_absent(text, tokens, label):
    for token in tokens:
        assert token not in text, f"Forbidden {label}: {token}"


def test_lora_gnss_dashboard_source():
    require_tokens(
        SOURCE,
        [
            "Screen::GnssDashboard",
            "{\"GNSS Dash\", Screen::GnssDashboard}",
            "GNSS_DASH_SERVICE_INTERVAL_MS = 200",
            "GNSS_DASH_RENDER_INTERVAL_MS = 1000",
            "showGnssDashboard()",
            "renderGnssDashboard()",
            "initGnssDashboard()",
            "serviceGnssDashboard();",
            "stopGnssDashboard();",
            "drawScreenFrame(\"GNSS Dashboard\")",
            "HardwareSerial loraGnssSerial(1)",
            "TinyGPSPlus loraGps",
            "resetLoraGnssParser()",
            "stopLoraGnssSerial()",
            "loraGps.speed.kmph()",
            "loraGps.altitude.meters()",
            "loraGnssSpeedText()",
            "loraGnssAltitudeText()",
            "loraGnssUtcText()",
            "loraGnssDateText()",
            "Speed:%s Alt:%s",
            "buildGnssDashboardOledLines",
        ],
        "GNSS dashboard source token",
    )

    assert_tokens_absent(
        RX_ONLY_SOURCE,
        [
            "startTransmit(",
            ".transmit(",
            "setPacketSentAction(",
            "#include <RF24.h>",
            "Screen::RfScanner",
            "showRfScanner",
        ],
        "active TX or retired RF Scan token",
    )


def test_lora_gnss_dashboard_docs():
    for doc_name, text in {
        "README.md": README,
        "notes.md": NOTES,
        "todo.md": TODO,
    }.items():
        require_tokens(
            text,
            [
                "GNSS Dash",
                "GNSS Dashboard",
                "Cap LoRa-1262",
                "TinyGPSPlus",
                "speed",
                "altitude",
                "UTC",
                "No transmit",
            ],
            f"GNSS dashboard documentation in {doc_name}",
        )


if __name__ == "__main__":
    test_lora_gnss_dashboard_source()
    test_lora_gnss_dashboard_docs()
    print("LoRa GNSS dashboard checks passed.")
