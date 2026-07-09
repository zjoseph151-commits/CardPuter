from pathlib import Path
import re


SOURCE = Path("src/main.cpp").read_text()


def function_body(name):
    start = SOURCE.index(f"void {name}() {{")
    next_function = SOURCE.index("\nvoid ", start + 1)
    return SOURCE[start:next_function]


def const_char_function_body(name):
    match = re.search(
        rf"const\s+char\*\s+{name}\s*\([^)]*\)\s*\{{",
        SOURCE,
        re.MULTILINE,
    )
    if not match:
      raise AssertionError(f"Could not find const char* function {name}")

    body_start = SOURCE.index("{", match.start())
    depth = 0

    for index in range(body_start, len(SOURCE)):
        char = SOURCE[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return SOURCE[body_start:index]

    raise AssertionError(f"Could not find body for {name}")


def test_system_screen_reports_charge_status():
    battery_screen = function_body("showBatteryInfo")
    system_screen = function_body("showSystemInfo")
    filtered_charge_text = const_char_function_body("inferredChargingText")

    assert "M5.Power.isCharging()" in SOURCE
    assert "M5.Power.getVBUSVoltage()" in SOURCE
    assert "Charge:" in SOURCE
    assert "FIRMWARE_VERSION" not in battery_screen
    assert "Firmware:" in system_screen
    assert "sampleBatteryStatus()" in SOURCE
    assert "updateBatteryTrend(" in SOURCE
    assert "CHARGING_TREND_THRESHOLD_MV = 50" in SOURCE
    assert "CHARGING_CONFIRM_SAMPLES = 3" in SOURCE
    assert "VBUS_PRESENT_THRESHOLD_MV" in SOURCE
    assert "voltageDelta > CHARGING_TREND_THRESHOLD_MV" in SOURCE
    assert "trendAboveThresholdSamples >= CHARGING_CONFIRM_SAMPLES" in SOURCE
    assert "levelDelta > 0" not in SOURCE
    assert "isExternalPowerPresent()" in SOURCE
    assert "isFilteredCharging()" in SOURCE
    assert "isBatteryLevelDisplayable()" in SOURCE
    assert "lastVbusVoltageMv >= VBUS_PRESENT_THRESHOLD_MV" in SOURCE
    assert "isExternalPowerPresent() &&" in SOURCE
    assert "lastChargingStatus == m5::Power_Class::is_charging" in SOURCE
    assert "!isFilteredCharging()" in SOURCE
    assert "status == m5::Power_Class::is_charging" not in filtered_charge_text
    assert "status == m5::Power_Class::is_discharging" not in filtered_charge_text
    assert "batteryTrend.lastVoltageMv - batteryTrend.minVoltageMv" in SOURCE
    assert "resetBatteryTrend();" not in SOURCE
    assert "voltageDelta >= 25" not in SOURCE
    assert 'return "Charging";' in SOURCE
    assert 'return "Not charging";' in SOURCE
    assert "Likely charging" not in SOURCE
    assert "Likely battery" not in SOURCE
    assert "Watching" not in SOURCE
    assert "Checking" not in SOURCE


if __name__ == "__main__":
    test_system_screen_reports_charge_status()
    print("Power status checks passed.")
