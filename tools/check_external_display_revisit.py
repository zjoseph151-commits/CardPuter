from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
README = ROOT / "README.md"
TODO = ROOT / "todo.md"
NOTES = ROOT / "notes.md"
PLATFORMIO = ROOT / "platformio.ini"
PLAN = ROOT / "docs" / "superpowers" / "plans" / "2026-08-15-external-display-revisit.md"

readme = README.read_text(encoding="utf-8")
todo = TODO.read_text(encoding="utf-8")
notes = NOTES.read_text(encoding="utf-8")
platformio = PLATFORMIO.read_text(encoding="utf-8")
plan = PLAN.read_text(encoding="utf-8")
source = firmware_source_text()

assert "olikraus/U8g2" in platformio, "OLED proof should use U8g2"

for token in [
    "OLED_SDA_PIN",
    "OLED_SCL_PIN",
]:
    assert token not in source, f"External OLED must not use direct pin token: {token}"

required_source_tokens = [
    "I2C_HUB_ADDRESS = 0x70",
    "I2C_HUB_CHANNEL_COUNT = 6",
    "I2C_HUB_ENV_CHANNEL = 0",
    "I2C_HUB_OLED_CHANNEL = 1",
    "I2C_HUB_CHANNEL_SETTLE_US = 1000",
    "detectI2cHub()",
    "selectI2cHubChannel(",
    "selectEnvironmentI2cPath()",
    "selectOledI2cPath()",
    "OLED_I2C_ADDRESS_PRIMARY = 0x3C",
    "OLED_I2C_ADDRESS_SECONDARY = 0x3D",
    "Screen::OledTest",
    "renderOledStatusDashboard()",
    "serviceOledStatusDashboard()",
    "setOledStatusLine(const String& line)",
    "drawOledTestPattern()",
    "Wire.write(1U << channel)",
    "delayMicroseconds(I2C_HUB_CHANNEL_SETTLE_US)",
    "PaHub ch",
]

for token in required_source_tokens:
    assert token in source, f"Missing PaHub firmware foundation token: {token}"

required_docs = {
    "README.md": [
        "Has an OLED Status Dashboard",
        "M5Stack Unit PaHub v2.1",
        "PCA9548AP",
        "Avoid using G8/G9 directly",
        "ENV III on PaHub channel 0",
        "SSD1309 OLED on PaHub channel 1",
        "Keep external I2C modules on Grove `G2/G1` unless there is a deliberate expansion/mux/buffer plan.",
    ],
    "todo.md": [
        "Priority #7 planning started on 2026-08-15.",
        "User chose the M5Stack Unit PaHub v2.1",
        "default PaHub address `0x70`",
        "ENV III on PaHub channel 0",
        "SSD1309 OLED on PaHub channel 1",
        "OLED Status Dashboard foundation",
        "OLED Test proof-of-life added",
        "Do not use G8/G9 directly for external I2C.",
    ],
    "notes.md": [
        "OLED Status Dashboard foundation is active.",
        "OLED Test proof-of-life screen remains active as diagnostics.",
        "U8g2 dependency is active for the dashboard and diagnostics screen.",
        "M5Stack Unit PaHub v2.1",
        "ENV III on PaHub channel 0",
        "SSD1309 OLED on PaHub channel 1",
    ],
    str(PLAN.relative_to(ROOT)): [
        "OLED Status Dashboard foundation",
        "OLED Test proof-of-life screen was added.",
        "Chosen path: M5Stack Unit PaHub v2.1",
        "Default I2C address: `0x70`",
        "ENV III on PaHub channel 0",
        "SSD1309 OLED on PaHub channel 1",
        "Detect the PaHub at `0x70`.",
        "Do not use G8/G9 directly for external I2C.",
        "Add a simple proof-of-life screen",
        "Verify arrow keys after wiring.",
    ],
}

doc_texts = {
    "README.md": readme,
    "todo.md": todo,
    "notes.md": notes,
    str(PLAN.relative_to(ROOT)): plan,
}

for doc_name, tokens in required_docs.items():
    text = doc_texts[doc_name]
    for token in tokens:
        assert token in text, f"Missing external display revisit note in {doc_name}: {token}"

print("External display revisit checks passed.")
