from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
SOURCE = firmware_source_text()
TRACKER = (ROOT / "src" / "satellite_pass_tracker.cpp").read_text(
    encoding="utf-8"
)
PLATFORMIO = (ROOT / "platformio.ini").read_text(encoding="utf-8")
CERT = (ROOT / "include" / "celestrak_ca.h").read_text(encoding="utf-8")


def require(text, *tokens):
    for token in tokens:
        assert token in text, f"Missing satellite pass token: {token}"


def test_integration():
    require(SOURCE, '{"Sat Passes", Screen::SatellitePassTracker}',
            "showSatellitePassTracker();", "serviceSatellitePassTracker();",
            "stopSatellitePassTracker();", "buildSatellitePassOledLines(lines)",
            "satellitePassEditingCatalog()")
    require(PLATFORMIO, "Hopperpop/Sgp4-Library.git#")
    require(CERT, "BEGIN CERTIFICATE", "Sectigo Public Server Authentication")


def test_cache_and_prediction():
    require(TRACKER, "CATNR=", "&FORMAT=TLE", "http.GET()",
            "client.setCACert(CELESTRAK_ROOT_CA)", "validTleLine(",
            "currentElementEpoch(", "DOWNLOAD_INTERVAL_SECONDS",
            'prefs.getUInt(attemptKey.c_str(), 0)',
            'prefs.putUInt(attemptKey.c_str()', '"/config/sat"',
            "SD.rename(temporary, path)", "loadSelectedCache()",
            "loraGnssHasFreshFix()", "loraGnssDateValid",
            "loraGnssTimeValid", "orbit.initpredpoint(",
            "orbit.nextpass(&pass, 32, false, 10.0)",
            "MAX_ELEMENTS_SECONDS", "riseUnix", "peakUnix", "setUnix")
    for token in ("SX1262", "startTransmit(", ".transmit(",
                  "WiFi.begin(", "setInsecure("):
        assert token not in TRACKER, f"Unexpected tracker behavior: {token}"


if __name__ == "__main__":
    test_integration()
    test_cache_and_prediction()
    print("Satellite pass tracker checks passed.")
