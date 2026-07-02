from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "src" / "main.cpp"
README = ROOT / "README.md"
PLATFORMIO = ROOT / "platformio.ini"

main = MAIN.read_text(encoding="utf-8")
readme = README.read_text(encoding="utf-8")
platformio = PLATFORMIO.read_text(encoding="utf-8")

forbidden_platformio_tokens = [
    "olikraus/U8g2",
]

for token in forbidden_platformio_tokens:
    assert token not in platformio, f"Remove inactive OLED dependency: {token}"

forbidden_main_tokens = [
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
    "probeOledAddress(",
    "oledOnline",
    "oledActiveAddress",
]

for token in forbidden_main_tokens:
    assert token not in main, f"Remove inactive OLED firmware token: {token}"

required_readme_tokens = [
    "Future idea",
    "SSD1309 OLED",
    "secondary display",
    "Avoid using G8/G9 directly",
]

for token in required_readme_tokens:
    assert token in readme, f"Missing future OLED note: {token}"

forbidden_readme_tokens = [
    "OLED Test",
    "SDA -> Cardputer G2",
    "SCL -> Cardputer G1",
]

for token in forbidden_readme_tokens:
    assert token not in readme, f"Remove active OLED instructions from README: {token}"

print("OLED future-note checks passed.")
