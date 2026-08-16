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
    "buildRfScanOledLines",
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
    "Tgt: ",
    "Cmd: ",
    "REC %02lu:%02lu",
    "Press: invalid",
    "Quiet:",
]

for token in required_source_tokens:
    assert token in source, f"Missing OLED status dashboard token: {token}"

forbidden_source_tokens = [
    "OLED_SDA_PIN",
    "OLED_SCL_PIN",
    "SDA -> Cardputer G2",
    "SCL -> Cardputer G1",
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
