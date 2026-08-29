from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
SOURCE = firmware_source_text()
RETURN_HOME = (ROOT / "src" / "return_home.cpp").read_text(encoding="utf-8")
README = (ROOT / "README.md").read_text(encoding="utf-8")
NOTES = (ROOT / "notes.md").read_text(encoding="utf-8")
TODO = (ROOT / "todo.md").read_text(encoding="utf-8")


def require_tokens(text, tokens, label):
    for token in tokens:
        assert token in text, f"Missing {label}: {token}"


def assert_tokens_absent(text, tokens, label):
    for token in tokens:
        assert token not in text, f"Forbidden {label}: {token}"


def test_return_home_source():
    require_tokens(
        SOURCE,
        [
            "Screen::ReturnHome",
            "{\"Return Home\", Screen::ReturnHome}",
            "RETURN_HOME_SERVICE_INTERVAL_MS = 200",
            "RETURN_HOME_RENDER_INTERVAL_MS = 1000",
            "RETURN_HOME_ARRIVAL_RADIUS_METERS = 10.0f",
            "RETURN_HOME_PREF_NAMESPACE = \"scoober_home\"",
            "showReturnHome()",
            "renderReturnHome()",
            "initReturnHome()",
            "serviceReturnHome();",
            "stopReturnHome();",
            "drawScreenFrame(\"Return Home\")",
            "loadReturnHomeWaypoint()",
            "saveReturnHomeWaypoint()",
            "clearReturnHomeWaypoint()",
            "updateReturnHomeNavigation()",
            "returnHomeDistanceText()",
            "returnHomeBearingText()",
            "buildReturnHomeOledLines",
            "key == 's' || key == 'S'",
            "key == 'd' || key == 'D'",
        ],
        "Return Home source token",
    )

    require_tokens(
        RETURN_HOME,
        [
            "startLoraGnssSerial()",
            "serviceLoraGnssSerial()",
            "stopLoraGnssSerial()",
            "Preferences prefs",
            "prefs.putDouble(\"lat\"",
            "prefs.putDouble(\"lon\"",
            "prefs.putBool(\"valid\", true)",
            "prefs.clear()",
            "RETURN_HOME_EARTH_RADIUS_METERS",
            "returnHomeDistanceBetween",
            "returnHomeBearingBetween",
            "atan2",
            "sin(",
            "cos(",
            "gnssSkyCompassDirection(bearing)",
            "Need fresh GNSS fix",
            "S save current fix",
            "S update D clear",
            "No transmit path",
        ],
        "Return Home implementation token",
    )

    assert_tokens_absent(
        RETURN_HOME,
        [
            "#include <RadioLib.h>",
            "SX1262",
            "loraRadio.begin",
            "prepareSharedExtSpiForLora",
            "SPI.begin(",
            "startReceive(",
            "readData(",
            "startTransmit(",
            ".transmit(",
            "setPacketSentAction(",
            "#include <RF24.h>",
            "Screen::RfScanner",
            "showRfScanner",
            "rfScan",
        ],
        "radio/SPI/TX token in Return Home",
    )

    assert_tokens_absent(
        SOURCE,
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


def test_return_home_docs():
    for doc_name, text in {
        "README.md": README,
        "notes.md": NOTES,
        "todo.md": TODO,
    }.items():
        require_tokens(
            text,
            [
                "Waypoint / Return Home",
                "Return Home",
                "distance",
                "bearing",
                "saved home",
                "S",
                "D",
                "No transmit",
            ],
            f"Return Home documentation in {doc_name}",
        )


if __name__ == "__main__":
    test_return_home_source()
    test_return_home_docs()
    print("Return Home checks passed.")
