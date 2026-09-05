from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
README = ROOT / "README.md"
PLATFORMIO = ROOT / "platformio.ini"

source = firmware_source_text()
readme = README.read_text(encoding="utf-8")
platformio = PLATFORMIO.read_text(encoding="utf-8")

required_platformio_tokens = [
    "olikraus/U8g2",
]

for token in required_platformio_tokens:
    assert token in platformio, f"Missing OLED dependency: {token}"

required_source_tokens = [
    "#include <U8g2lib.h>",
    "Screen::OledTest",
    '"OLED Test"',
    "OLED_I2C_ADDRESS_PRIMARY",
    "OLED_I2C_ADDRESS_SECONDARY",
    "OLED_I2C_FAST_FREQUENCY",
    "OLED_I2C_FALLBACK_FREQUENCY = 100000U",
    "OLED_PROBE_ATTEMPTS = 3",
    "I2C_HUB_OLED_CHANNEL = 1",
    "U8G2_SSD1309_128X64_NONAME0_F_HW_I2C",
    "initOledDisplay()",
    "ensureOledReady()",
    "drawOledTestPattern()",
    "renderOledTest()",
    "renderOledStatusDashboard()",
    "serviceOledStatusDashboard()",
    "setOledStatusLine",
    "probeOledAddress(",
    "selectOledI2cPath()",
    "oledI2cPathLabel()",
    "oledOnline",
    "oledActiveAddress",
    "oledActiveBusFrequency",
    "oledScanSummary",
    "configureExternalI2cForOled",
    "restoreExternalI2cBusClock",
    "oledProbeFailureSummary",
    "tryOledProbeAtFrequency",
    "OLED proof-of-life",
    "Bus:",
    "Scan:",
    "OK/R retry",
]

for token in required_source_tokens:
    assert token in source, f"Missing OLED proof token: {token}"

forbidden_source_tokens = [
    "OLED_SDA_PIN",
    "OLED_SCL_PIN",
    "SDA -> Cardputer G2",
    "SCL -> Cardputer G1",
    "Wire.end()",
    "oledHubScanSummary",
    "scanOledCandidateChannels",
    "PaHub channel scan",
    "OLED not on ch0-5",
]

for token in forbidden_source_tokens:
    assert token not in source, f"Do not add direct OLED wiring token: {token}"

required_readme_tokens = [
    "OLED Test",
    "OLED Status Dashboard",
    "SSD1309 OLED",
    "proof-of-life",
    "PaHub channel 1",
    "Avoid using G8/G9 directly",
    "OK/Enter or R retries OLED detection",
    "OLED fallback I2C frequency: `OLED_I2C_FALLBACK_FREQUENCY = 100000U`",
    "Shows the active OLED address, bus speed, and a short PaHub channel 1 probe summary",
    "0x3C",
    "0x3D",
]

for token in required_readme_tokens:
    assert token in readme, f"Missing README OLED proof note: {token}"

forbidden_readme_tokens = [
    "SDA -> Cardputer G2",
    "SCL -> Cardputer G1",
    "Has no active external OLED display code.",
]

for token in forbidden_readme_tokens:
    assert token not in readme, f"Remove stale/direct OLED README note: {token}"

print("OLED proof-of-life checks passed.")
