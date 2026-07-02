from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "src" / "main.cpp"
README = ROOT / "README.md"
PLATFORMIO = ROOT / "platformio.ini"

main = MAIN.read_text(encoding="utf-8")
readme = README.read_text(encoding="utf-8")
platformio = PLATFORMIO.read_text(encoding="utf-8")

required_platformio_tokens = [
    "m5stack/M5Unit-ENV",
]

for token in required_platformio_tokens:
    assert token in platformio, f"Missing PlatformIO ENV dependency: {token}"

required_main_tokens = [
    "#include <Wire.h>",
    "#include <M5UnitENV.h>",
    "ENV_I2C_SDA_PIN = 2",
    "ENV_I2C_SCL_PIN = 1",
    "ENV_I2C_FREQUENCY = 400000",
    "SHT3X envSht30",
    "QMP6988 envQmp6988",
    "Screen::Environment",
    '"Environment"',
    "showEnvironment()",
    "initEnvironmentSensor()",
    "readEnvironmentSensor()",
    "envTemperatureC",
    "envHumidityPercent",
    "envPressureHpa",
    "ENV III not found",
    "Temp:",
    "Humidity:",
    "Pressure:",
]

for token in required_main_tokens:
    assert token in main, f"Missing expected Environment token: {token}"

required_readme_tokens = [
    "M5Stack ENV III Unit",
    "Environment",
    "Grove",
    "temperature",
    "humidity",
    "pressure",
    "SDA -> Grove SDA",
    "SCL -> Grove SCL",
]

for token in required_readme_tokens:
    assert token in readme, f"Missing README Environment note: {token}"

print("Environment feature checks passed.")
