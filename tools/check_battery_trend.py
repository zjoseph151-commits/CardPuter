import re

from firmware_source import firmware_source_text


SOURCE = firmware_source_text()
threshold_match = re.search(r"CHARGING_TREND_THRESHOLD_MV = (\d+)", SOURCE)
assert threshold_match, "Missing CHARGING_TREND_THRESHOLD_MV"
CHARGING_TREND_THRESHOLD_MV = int(threshold_match.group(1))


required_tokens = [
    "CHARGING_CLEAR_SAMPLES",
    "fallingSamples >= CHARGING_CLEAR_SAMPLES",
    "batteryTrend.maxVoltageMv - batteryTrend.lastVoltageMv",
    "batteryTrend.inferredCharging = false",
    "batteryTrend.trendAboveThresholdSamples = 0",
]

for token in required_tokens:
    assert token in SOURCE, f"Missing battery trend clear token: {token}"

assert CHARGING_TREND_THRESHOLD_MV >= 50, (
    "Battery charging inference threshold should be above observed unplugged +26 mV drift"
)
assert "levelDelta > 0" not in SOURCE, (
    "Battery percentage is voltage-derived and should not infer charging by itself"
)


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
    threshold_mv = CHARGING_TREND_THRESHOLD_MV
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
    if voltage_delta > threshold_mv:
        trend.trend_above_threshold_samples += 1
    else:
        trend.trend_above_threshold_samples = 0
        trend.inferred_charging = False

    if trend.trend_above_threshold_samples >= confirm_samples:
        trend.inferred_charging = True


def test_charge_stop_clears_latched_inference():
    trend = BatteryTrend()

    for voltage in (3800, 3830, 3865, 3895, 3905):
        update_trend(trend, voltage, 50)

    assert trend.inferred_charging, "Rising voltage samples should infer charging"

    for voltage in (3865, 3835, 3805):
        update_trend(trend, voltage, 50)

    assert not trend.inferred_charging, (
        "A confirmed drop from the peak should clear inferred charging"
    )
    assert trend.min_voltage_mv == 3805, "Trend baseline should restart after clearing"


def test_unplugged_small_voltage_drift_does_not_infer_charging():
    trend = BatteryTrend()

    for voltage in (3800, 3809, 3818, 3826, 3826, 3826):
        update_trend(trend, voltage, 50)

    assert not trend.inferred_charging, (
        "A stable unplugged +26 mV drift should stay below the charging threshold"
    )


def test_percentage_bounce_does_not_infer_charging_without_voltage_trend():
    trend = BatteryTrend()

    for battery_level in (50, 70, 70, 70):
        update_trend(trend, 3800, battery_level)

    assert not trend.inferred_charging, (
        "Battery percentage jumps should not infer charging without a voltage trend"
    )


if __name__ == "__main__":
    test_charge_stop_clears_latched_inference()
    test_unplugged_small_voltage_drift_does_not_infer_charging()
    test_percentage_bounce_does_not_infer_charging_without_voltage_trend()
    print("Battery trend checks passed.")
