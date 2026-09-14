from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
README = ROOT / "README.md"
TODO = ROOT / "todo.md"
NOTES = ROOT / "notes.md"
RTC_SOURCE = ROOT / "src" / "rtc_status.cpp"

source = firmware_source_text()
rtc_source = RTC_SOURCE.read_text(encoding="utf-8")
readme = README.read_text(encoding="utf-8")
todo = TODO.read_text(encoding="utf-8")
notes = NOTES.read_text(encoding="utf-8")

required_source_tokens = [
    "Screen::RtcStatus",
    "{\"RTC\", Screen::RtcStatus}",
    "I2C_HUB_ENV_CHANNEL = 0",
    "I2C_HUB_OLED_CHANNEL = 1",
    "I2C_HUB_RTC_CHANNEL = 5",
    "RTC_DS3231_ADDRESS = 0x68",
    "RTC_AT24C32_ADDRESS = 0x57",
    "RTC_DS3231_CONTROL_REGISTER = 0x0E",
    "RTC_REFRESH_INTERVAL_MS = 1000",
    "RTC_RETRY_INTERVAL_MS = 3000",
    "RTC_NTP_SYNC_TIMEOUT_MS = 10000",
    "RTC_NTP_POLL_MS = 250",
    "RTC_TIMEZONE_POSIX",
    "RTC_NTP_SERVER_PRIMARY",
    "selectRtcI2cPath()",
    "showRtcStatus()",
    "renderRtcStatus()",
    "initRtcStatus()",
    "serviceRtcStatus();",
    "readRtcStatus()",
    "setRtcToBuildTime()",
    "setRtcFromNtp()",
    "drawScreenFrame(\"RTC (PaHub ch5)\")",
    "Wire.begin(ENV_I2C_SDA_PIN, ENV_I2C_SCL_PIN, ENV_I2C_FREQUENCY)",
    "WiFi.status() != WL_CONNECTED",
    "Use WiFi Connect",
    "configTzTime",
    "getLocalTime",
    "bcdToDecimal",
    "decimalToBcd",
    "advanceRtcDateTimeBySeconds",
    "RTC needs set",
    "RTC set build",
    "RTC set NTP",
    "N NTP S build OK/R",
    "N NTP EE:",
    "rtcReadAttemptCount",
    "buildRtcStatusOledLines",
    "buildRtcStatusOledHelpLines",
    "N set from NTP",
    "S set build time",
    "rtcDateText()",
    "rtcTimeText()",
]

for token in required_source_tokens:
    assert token in source, f"Missing RTC source token: {token}"

forbidden_rtc_tokens = [
    "SPI.begin",
    "SD.begin",
    "startLoraGnssSerial",
    "serviceLoraGnssSerial",
    "SX1262",
    "RadioLib",
    "startTransmit",
    ".transmit",
    "setPacketSentAction",
    "#include <RF24.h>",
    "RF24",
    "loraRadio",
    "WiFi.begin",
    "OLED_SDA_PIN",
    "OLED_SCL_PIN",
]

for token in forbidden_rtc_tokens:
    assert token not in rtc_source, f"RTC must stay I2C-only, found: {token}"

forbidden_source_tokens = [
    "Screen::RfScanner",
    "showRfScanner",
    "rfScan",
    "#include <RF24.h>",
]

for token in forbidden_source_tokens:
    assert token not in source, f"Retired NRF24/RF Scan token returned: {token}"

for doc_name, text in {
    "README.md": readme,
    "todo.md": todo,
    "notes.md": notes,
}.items():
    for token in [
        "Priority #9",
        "RTC",
        "DS3231",
        "AT24C32",
        "PaHub channel 5",
        "I2C_HUB_RTC_CHANNEL = 5",
        "RTC_DS3231_ADDRESS = 0x68",
        "RTC_AT24C32_ADDRESS = 0x57",
        "missing RTC",
        "EEPROM unused",
        "NTP",
        "WiFi Connect",
    ]:
        assert token in text, f"Missing RTC documentation token in {doc_name}: {token}"

print("RTC status checks passed.")
