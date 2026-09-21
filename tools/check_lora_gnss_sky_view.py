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


def test_lora_gnss_sky_view_source():
    require_tokens(
        SOURCE,
        [
            "Screen::GnssSkyView",
            "{\"GNSS Sky\", Screen::GnssSkyView}",
            "GNSS_SKY_MAX_SATELLITES = 24",
            "GNSS_SKY_STALE_MS = 15000",
            "GNSS_SKY_SERVICE_INTERVAL_MS = 200",
            "GNSS_SKY_RENDER_INTERVAL_MS = 1000",
            "struct GnssSkySatellite",
            "GnssSkySatellite gnssSkySatellites[GNSS_SKY_MAX_SATELLITES]",
            "gnssSkySatellitesInView",
            "gnssSkyGsvSentenceCount",
            "lastGnssSkyGsvMs",
            "selectedGnssSkySatelliteIndex",
            "isGsvSentence",
            "parseGsvSentence",
            "nmeaIntField",
            "gsvConstellation",
            "refreshGnssSkySatellites()",
            "clampGnssSkySelection()",
            "moveGnssSkySelection",
            "selectedGnssSkySatellite()",
            "gnssSkyConstellationName",
            "gnssSkyCompassDirection",
            "gnssSkySatelliteAgeText",
            "showGnssSkyView()",
            "renderGnssSkyView()",
            "initGnssSkyView()",
            "serviceGnssSkyView();",
            "stopGnssSkyView();",
            "drawScreenFrame(\"GNSS Sky View\")",
            "drawGnssSkyGrid",
            "drawGnssSkySatellite",
            "drawCircle(satX, satY, 7, WHITE)",
            "sinf(azimuthRad)",
            "cosf(azimuthRad)",
            "gnssSkySnrColor",
            "buildGnssSkyViewOledLines",
            "Sel: none",
            "PRN:",
            "SNR:",
            "El:",
            "Az:",
            "age:",
            "key == ';' || key == ','",
            "key == '.' || key == '/'",
            "renderOledStatusDashboard();",
        ],
        "GNSS sky view source token",
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


def test_lora_gnss_sky_view_docs():
    for doc_name, text in {
        "README.md": README,
        "notes.md": NOTES,
        "todo.md": TODO,
    }.items():
        require_tokens(
            text,
            [
                "GNSS Sky",
                "GNSS Satellite Sky View",
                "GSV",
                "elevation",
                "azimuth",
                "SNR",
                "selected satellite",
                "compass",
                "OLED",
                "No transmit",
            ],
            f"GNSS sky view documentation in {doc_name}",
        )


if __name__ == "__main__":
    test_lora_gnss_sky_view_source()
    test_lora_gnss_sky_view_docs()
    print("LoRa GNSS sky view checks passed.")
