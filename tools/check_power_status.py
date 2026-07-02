from pathlib import Path


SOURCE = Path("src/main.cpp").read_text()


def function_body(name):
    start = SOURCE.index(f"void {name}() {{")
    next_function = SOURCE.index("\nvoid ", start + 1)
    return SOURCE[start:next_function]


def test_system_screen_reports_charge_status():
    battery_screen = function_body("showBatteryInfo")
    system_screen = function_body("showSystemInfo")

    assert "M5.Power.isCharging()" in SOURCE
    assert "M5.Power.getVBUSVoltage()" in SOURCE
    assert "Charge:" in SOURCE
    assert "FIRMWARE_VERSION" not in battery_screen
    assert "Firmware:" in system_screen
    assert "sampleBatteryStatus()" in SOURCE
    assert "updateBatteryTrend(" in SOURCE
    assert "CHARGING_TREND_THRESHOLD_MV = 20" in SOURCE
    assert "CHARGING_CONFIRM_SAMPLES = 3" in SOURCE
    assert "voltageDelta > CHARGING_TREND_THRESHOLD_MV" in SOURCE
    assert "trendAboveThresholdSamples >= CHARGING_CONFIRM_SAMPLES" in SOURCE
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
