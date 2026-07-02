from pathlib import Path


SOURCE = Path("src/main.cpp").read_text()


required_tokens = [
    "CHARGING_CLEAR_SAMPLES",
    "fallingSamples >= CHARGING_CLEAR_SAMPLES",
    "batteryTrend.maxVoltageMv - batteryTrend.lastVoltageMv",
    "batteryTrend.inferredCharging = false",
    "batteryTrend.trendAboveThresholdSamples = 0",
]

for token in required_tokens:
    assert token in SOURCE, f"Missing battery trend clear token: {token}"


class BatteryTrend:
    def __init__(self):
        self.initialized = False
        self.first_voltage_mv = 0
        self.last_voltage_mv = 0
        self.min_voltage_mv = 0
        self.max_voltage_mv = 0
        self.first_level = -1
        self.last_level = -1
        self.rising_samples = 0
        self.falling_samples = 0
        self.trend_above_threshold_samples = 0
        self.inferred_charging = False


def update_trend(trend, voltage_mv, battery_level):
    threshold_mv = 20
    confirm_samples = 3
    clear_samples = 3

    if voltage_mv <= 0:
        return

    if not trend.initialized:
        trend.initialized = True
        trend.first_voltage_mv = voltage_mv
        trend.last_voltage_mv = voltage_mv
        trend.min_voltage_mv = voltage_mv
        trend.max_voltage_mv = voltage_mv
        trend.first_level = battery_level
        trend.last_level = battery_level
        return

    if voltage_mv > trend.last_voltage_mv + 2:
        trend.rising_samples += 1
        trend.falling_samples = 0
    elif voltage_mv < trend.last_voltage_mv - 2:
        trend.falling_samples += 1
        trend.rising_samples = 0

    trend.last_voltage_mv = voltage_mv
    trend.last_level = battery_level
    trend.min_voltage_mv = min(trend.min_voltage_mv, voltage_mv)
    trend.max_voltage_mv = max(trend.max_voltage_mv, voltage_mv)

    confirmed_drop_from_peak = (
        trend.falling_samples >= clear_samples
        and trend.max_voltage_mv - trend.last_voltage_mv > threshold_mv
    )
    if confirmed_drop_from_peak:
        trend.first_voltage_mv = voltage_mv
        trend.min_voltage_mv = voltage_mv
        trend.max_voltage_mv = voltage_mv
        trend.first_level = battery_level
        trend.rising_samples = 0
        trend.falling_samples = 0
        trend.trend_above_threshold_samples = 0
        trend.inferred_charging = False
        return

    voltage_delta = trend.last_voltage_mv - trend.min_voltage_mv
    level_delta = (
        trend.last_level - trend.first_level
        if trend.first_level >= 0 and trend.last_level >= 0
        else 0
    )

    if level_delta > 0 or voltage_delta > threshold_mv:
        trend.trend_above_threshold_samples += 1
    else:
        trend.trend_above_threshold_samples = 0
        trend.inferred_charging = False

    if trend.trend_above_threshold_samples >= confirm_samples:
        trend.inferred_charging = True


def test_charge_stop_clears_latched_inference():
    trend = BatteryTrend()

    for voltage in (3800, 3812, 3830, 3845, 3860):
        update_trend(trend, voltage, 50)

    assert trend.inferred_charging, "Rising voltage samples should infer charging"

    for voltage in (3838, 3825, 3810):
        update_trend(trend, voltage, 50)

    assert not trend.inferred_charging, (
        "A confirmed drop from the peak should clear inferred charging"
    )
    assert trend.min_voltage_mv == 3810, "Trend baseline should restart after clearing"


if __name__ == "__main__":
    test_charge_stop_clears_latched_inference()
    print("Battery trend checks passed.")
