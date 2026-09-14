from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
README = ROOT / "README.md"
TODO = ROOT / "todo.md"
NOTES = ROOT / "notes.md"
PLAN = ROOT / "docs" / "superpowers" / "plans" / "2026-08-15-external-display-revisit.md"

source = firmware_source_text()
readme = README.read_text(encoding="utf-8")
todo = TODO.read_text(encoding="utf-8")
notes = NOTES.read_text(encoding="utf-8")
plan = PLAN.read_text(encoding="utf-8")

required_source_tokens = [
    "I2C_HUB_ENV_CHANNEL = 0",
    "I2C_HUB_OLED_CHANNEL = 1",
    "OLED_STATUS_REFRESH_INTERVAL_MS = 1000",
    "OLED_RETRY_INTERVAL_MS = 3000",
    "OLED_STATUS_LINE_COUNT = 5",
    "OLED_STATUS_MAX_CHARS = 21",
    "PI_MONITOR_OLED_MESSAGE_LINE_COUNT = 8",
    "PI_MONITOR_OLED_MESSAGE_MAX_CHARS = 25",
    "OLED_HELP_LINE_COUNT = PI_MONITOR_OLED_MESSAGE_LINE_COUNT",
    "OLED_HELP_MAX_CHARS = PI_MONITOR_OLED_MESSAGE_MAX_CHARS",
    "serviceOledStatusDashboard()",
    "renderOledStatusDashboard()",
    "setOledStatusLine(const String& line)",
    "clearOledStatusLine()",
    "ensureOledReady()",
    "buildOledDashboardLines",
    "buildMainMenuOledLines",
    "buildPiMonitorOledLines",
    "buildEnvironmentOledLines",
    "buildVoiceMemoOledLines",
    "buildRtcStatusOledLines",
    "buildWifiOledLines",
    "buildLoraDiagOledLines",
    "buildReturnHomeOledLines",
    "oledBatteryLine()",
    "oledWifiLine()",
    "oledMqttLine()",
    "serviceOledStatusDashboard();",
    "selectOledI2cPath()",
    "selectEnvironmentI2cPath()",
    "Battery:",
    "WiFi:",
    "MQTT:",
    "Mode: Menu",
    "renderPiMonitorOledMessages",
    "buildPiMonitorOledWrappedLines",
    "drawPiMonitorOledMessageLine",
    "drawOledHelpLine",
    "renderOledHelpLines",
    "renderOledHelpDashboard()",
    "buildWifiConnectOledHelpLines",
    "buildPiMonitorOledHelpLines",
    "buildSdManagerOledHelpLines",
    "buildRtcStatusOledHelpLines",
    "buildGnssDashboardOledHelpLines",
    "buildGnssSkyViewOledHelpLines",
    "buildReturnHomeOledHelpLines",
    "buildBreadcrumbLoggerOledHelpLines",
    "buildLoraPacketMonitorOledHelpLines",
    "buildLoraDiagOledHelpLines",
    "piMonitorLatestOledMessageText",
    "piMonitorLatestOledMessageSequence",
    "advancePiMonitorOledMessagePage",
    "u8g2_font_5x7_tf",
    "Uses /config/wifi.txt",
    "Arw choose project",
    "I info N new D del",
    "N set from NTP",
    "No LoRa radio/TX",
    "Arw select satellite",
    "Guides to saved home",
    "Logs GNSS CSV tracks",
    "Match sender settings",
    "Cap LoRa/GNSS check",
    "MQTT ",
    "REC %02lu:%02lu",
    "Press: invalid",
    "DS3231:",
    "N NTP EE:",
    "RX only No TX",
    "Return Home",
    "Dist: ",
    "Bear: ",
    "oledLevelLine()",
]

for token in required_source_tokens:
    assert token in source, f"Missing OLED status dashboard token: {token}"

forbidden_source_tokens = [
    "OLED_SDA_PIN",
    "OLED_SCL_PIN",
    "SDA -> Cardputer G2",
    "SCL -> Cardputer G1",
    "buildRfScanOledLines",
    "Screen::RfScanner",
    "rfScan",
    "PI_MONITOR_OLED_PAGE_INTERVAL_MS",
]

for token in forbidden_source_tokens:
    assert token not in source, f"Do not add direct OLED wiring token: {token}"

required_readme_tokens = [
    "OLED Status Dashboard",
    "Current screen/mode",
    "Battery level or charging status",
    "Wi-Fi connected/disconnected",
    "MQTT connected/disconnected",
    "setOledStatusLine",
    "renderOledStatusDashboard()",
    "serviceOledStatusDashboard()",
    "OLED remains on PaHub channel 1",
    "ENV III remains on PaHub channel 0",
]

for token in required_readme_tokens:
    assert token in readme, f"Missing README OLED status dashboard note: {token}"

for doc_name, text in {
    "todo.md": todo,
    "notes.md": notes,
    str(PLAN.relative_to(ROOT)): plan,
}.items():
    for token in [
        "OLED Status Dashboard",
        "PaHub channel 1",
        "ENV III on PaHub channel 0",
        "Do not use G8/G9 directly",
    ]:
        assert token in text, f"Missing OLED dashboard note in {doc_name}: {token}"

print("OLED status dashboard checks passed.")
