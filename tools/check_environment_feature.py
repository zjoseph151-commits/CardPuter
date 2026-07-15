from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
README = ROOT / "README.md"
PLATFORMIO = ROOT / "platformio.ini"

main = firmware_source_text()
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
    "#include <SD.h>",
    "ENV_I2C_SDA_PIN = 2",
    "ENV_I2C_SCL_PIN = 1",
    "ENV_I2C_FREQUENCY = 400000",
    'ENV_LOG_DIR = "/env"',
    "ENV_LOG_HEADER",
    "ENV_LOG_NAME_MAX_LENGTH",
    "Wire.begin(ENV_I2C_SDA_PIN, ENV_I2C_SCL_PIN, ENV_I2C_FREQUENCY)",
    "SHT3X envSht30",
    "QMP6988 envQmp6988",
    "Screen::Environment",
    "Screen::EnvironmentLogName",
    '"Environment"',
    "renderEnvironmentLogName()",
    "showEnvironment()",
    "initEnvironmentSensor()",
    "readEnvironmentSensor()",
    "initEnvironmentLogSd()",
    "findNextEnvironmentLogPath(",
    "sanitizeEnvironmentLogName(",
    "startEnvironmentLogging()",
    "startEnvironmentLogging(const String& requestedName)",
    "stopEnvironmentLogging(",
    "appendEnvironmentLogSample()",
    "envLogNameInput",
    "envTemperatureC",
    "envHumidityPercent",
    "envPressureHpa",
    "envLogSampleCount",
    "envLogging",
    "ENV III not found",
    "Temp:",
    "Humidity:",
    "Pressure:",
    "L name log",
    "L stop log",
    "Name this log:",
    "OK start",
    "Back del/cancel",
    "uptime_s,temp_c,temp_f,humidity_pct,pressure_hpa,altitude_m",
]

for token in required_main_tokens:
    assert token in main, f"Missing expected Environment token: {token}"

required_readme_tokens = [
    "M5Stack ENV III Unit",
    "Environment",
    "Grove",
    "CSV",
    "L to name and start logging",
    "OK/Enter starts logging",
    "Backspace deletes characters",
    "/env/env001.csv",
    "/env/backyard001.csv",
    "uptime_s,temp_c,temp_f,humidity_pct,pressure_hpa,altitude_m",
    "temperature",
    "humidity",
    "pressure",
    "SDA -> Grove SDA",
    "SCL -> Grove SCL",
]

for token in required_readme_tokens:
    assert token in readme, f"Missing README Environment note: {token}"

print("Environment feature checks passed.")
