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

for token in [
    "olikraus/U8g2",
]:
    assert token not in platformio, f"External display is inactive; remove dependency: {token}"

for token in [
    "#include <U8g2lib.h>",
    "Screen::OledTest",
    '"OLED Test"',
    "OLED_SDA_PIN",
    "OLED_SCL_PIN",
    "OLED_I2C_ADDRESS_PRIMARY",
    "OLED_I2C_ADDRESS_SECONDARY",
    "U8G2_SSD1309_128X64_NONAME0_F_HW_I2C",
    "initOledDisplay()",
    "drawOledTestPattern()",
    "renderOledTest()",
]:
    assert token not in source, f"External display is inactive; remove firmware token: {token}"

required_source_tokens = [
    "I2C_HUB_ADDRESS = 0x70",
    "I2C_HUB_CHANNEL_COUNT = 6",
    "I2C_HUB_ENV_CHANNEL = 0",
    "I2C_HUB_OLED_CHANNEL = 1",
    "detectI2cHub()",
    "selectI2cHubChannel(",
    "selectEnvironmentI2cPath()",
    "Wire.write(1U << channel)",
    "PaHub ch",
]

for token in required_source_tokens:
    assert token in source, f"Missing PaHub firmware foundation token: {token}"

required_docs = {
    "README.md": [
        "Has no active external OLED display code.",
        "M5Stack Unit PaHub v2.1",
        "PCA9548AP",
        "Avoid using G8/G9 directly",
        "safe pin plan or I2C expansion path",
        "ENV III on PaHub channel 0",
        "OLED reserved for PaHub channel 1",
        "Keep external I2C modules on Grove `G2/G1` unless there is a deliberate expansion/mux/buffer plan.",
    ],
    "todo.md": [
        "Priority #7 planning started on 2026-08-15.",
        "User chose the M5Stack Unit PaHub v2.1",
        "default PaHub address `0x70`",
        "ENV III on PaHub channel 0",
        "OLED reserved for PaHub channel 1",
        "External display support is intentionally inactive.",
        "Do not use G8/G9 directly for external I2C.",
        "choose safe pins or use Grove with an I2C mux/expander",
        "add U8g2 back only when the hardware plan is approved",
    ],
    "notes.md": [
        "No active OLED code.",
        "No U8g2 dependency.",
        "M5Stack Unit PaHub v2.1",
        "ENV III on PaHub channel 0",
        "OLED reserved for PaHub channel 1",
        "Reintroduce display support only when there is a deliberate pin plan.",
    ],
    str(PLAN.relative_to(ROOT)): [
        "No active external display code should be added until the hardware plan is approved.",
        "Chosen path: M5Stack Unit PaHub v2.1",
        "Default I2C address: `0x70`",
        "ENV III on PaHub channel 0",
        "OLED reserved for PaHub channel 1",
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
