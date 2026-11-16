from pathlib import Path
import re
import ssl

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
SOURCE = firmware_source_text()
TRACKER = (ROOT / "src" / "flight_tracker.cpp").read_text(encoding="utf-8")
CERT = (ROOT / "include" / "adsb_lol_ca.h").read_text(encoding="utf-8")


def require(text, *tokens):
    for token in tokens:
        assert token in text, f"Missing Flight Tracker token: {token}"


def test_integration():
    require(
        SOURCE,
        '{"Flight Tracker", Screen::FlightTracker}',
        "showFlightTracker();",
        "serviceFlightTracker();",
        "stopFlightTracker();",
        "requestFlightTracker();",
        "buildFlightTrackerOledLines(lines)",
        "closeFlightDetail()",
    )
    require(CERT, "BEGIN CERTIFICATE", "ISRG Root X1")
    pem = re.search(
        r"-----BEGIN CERTIFICATE-----[\s\S]*?-----END CERTIFICATE-----", CERT
    )
    assert pem is not None
    assert len(ssl.PEM_cert_to_DER_cert(pem.group())) > 1000


def test_request_boundary():
    require(
        TRACKER,
        "loraGnssHasFreshFix()",
        "WiFi.status() != WL_CONNECTED",
        "gnssUtc(utc)",
        "client.setCACert(ADSB_LOL_ROOT_CA)",
        "https://api.adsb.lol/v2/lat/",
        "SEARCH_RADIUS_NM = 25",
        "MAX_RESPONSE_BYTES = 65536",
        "MAX_AIRCRAFT_JSON_BYTES = 3072",
        "AIRCRAFT_DOCUMENT_BYTES = 1536",
        "RESPONSE_IDLE_TIMEOUT_MS = 20000",
        "MAX_AIRCRAFT = 12",
        "REQUEST_COOLDOWN_MS = 60000",
        "http.getSize()",
        "BoundedJsonReader reader(stream, length, receivedBytes)",
        "input_.available()",
        "input_.read(buffer_, requested)",
        "body stopped after %lu/%lu bytes",
        'String("API short ") + receivedBytes',
        "readAircraftObject(reader, staleCount, unpositionedCount)",
        "StaticJsonDocument<AIRCRAFT_DOCUMENT_BYTES> document",
        "DeserializationOption::Filter(filter)",
        "DeserializationOption::NestingLimit(5)",
        "retainNearest(candidate)",
        "Not for navigation",
        "Stale data; R refresh",
        "No aircraft in range",
        "API rate limited (429)",
    )
    assert "DynamicJsonDocument" not in TRACKER
    assert "readBytes(" not in TRACKER
    for forbidden in (
        "WiFi.begin(",
        "connectWifiFromConfig(",
        "SD.begin(",
        "SD.open(",
        "SX1262",
        "startTransmit(",
        ".transmit(",
        "setInsecure(",
    ):
        assert forbidden not in TRACKER, f"Unexpected Flight Tracker behavior: {forbidden}"


if __name__ == "__main__":
    test_integration()
    test_request_boundary()
    print("Flight Tracker checks passed.")
