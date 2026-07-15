#include "app.h"

void showBatteryInfo() {
  lastSystemRefreshMs = millis();

  beginContentDraw();

  if (lastBatteryVoltageMv > 0) {
    contentCanvas.printf("Batt: %d mV", lastBatteryVoltageMv);
  } else {
    contentCanvas.print("Batt: unavailable");
  }

  if (isBatteryLevelDisplayable()) {
    contentCanvas.printf(" %d%%\n", lastBatteryLevel);
  } else {
    contentCanvas.println(" --%");
  }

  contentCanvas.printf("Charge: %s\n", inferredChargingText(lastChargingStatus));
  contentCanvas.printf("API: %s\n", chargingStatusText(lastChargingStatus));

  if (lastVbusVoltageMv >= 0) {
    contentCanvas.printf("VBUS:%d mV ", lastVbusVoltageMv);
  } else {
    contentCanvas.print("VBUS:n/a ");
  }

  if (lastBatteryCurrentMa != 0) {
    contentCanvas.printf("I:%+dmA\n", lastBatteryCurrentMa);
  } else {
    contentCanvas.println("I:n/a");
  }

  if (batteryTrend.initialized && lastBatteryVoltageMv > 0) {
    contentCanvas.printf("Trend: %+d mV",
                         batteryTrend.lastVoltageMv - batteryTrend.minVoltageMv);
    if (lastBatteryLevel >= 0 && batteryTrend.firstLevel >= 0) {
      contentCanvas.printf(" %+d%%", batteryTrend.lastLevel - batteryTrend.firstLevel);
    }
    contentCanvas.println();
  }

  commitContentDraw();
}

void showSystemInfo() {
  lastSystemRefreshMs = millis();

  beginContentDraw();
  contentCanvas.printf("Firmware: %s\n", FIRMWARE_VERSION);
  contentCanvas.printf("Uptime: %lu sec\n", millis() / 1000UL);
  contentCanvas.printf("Free heap: %u\n", ESP.getFreeHeap());
  contentCanvas.printf("CPU: %u MHz\n", ESP.getCpuFreqMHz());
  contentCanvas.printf("Flash: %u MB\n", ESP.getFlashChipSize() / (1024U * 1024U));
  contentCanvas.printf("Screen: %d x %d\n", M5Cardputer.Display.width(),
                       M5Cardputer.Display.height());
  commitContentDraw();
}

void sampleBatteryStatus() {
  lastBatterySampleMs = millis();
  lastBatteryVoltageMv = M5.Power.getBatteryVoltage();
  lastBatteryLevel = M5.Power.getBatteryLevel();
  lastVbusVoltageMv = M5.Power.getVBUSVoltage();
  lastBatteryCurrentMa = M5.Power.getBatteryCurrent();
  lastChargingStatus = M5.Power.isCharging();

  updateBatteryTrend(lastBatteryVoltageMv, lastBatteryLevel);
}

void updateBatteryTrend(int voltageMv, int batteryLevel) {
  if (voltageMv <= 0) {
    return;
  }

  if (!batteryTrend.initialized) {
    batteryTrend.initialized = true;
    batteryTrend.firstVoltageMv = voltageMv;
    batteryTrend.lastVoltageMv = voltageMv;
    batteryTrend.minVoltageMv = voltageMv;
    batteryTrend.maxVoltageMv = voltageMv;
    batteryTrend.firstLevel = batteryLevel;
    batteryTrend.lastLevel = batteryLevel;
    return;
  }

  if (voltageMv > batteryTrend.lastVoltageMv + 2) {
    batteryTrend.risingSamples++;
    batteryTrend.fallingSamples = 0;
  } else if (voltageMv < batteryTrend.lastVoltageMv - 2) {
    batteryTrend.fallingSamples++;
    batteryTrend.risingSamples = 0;
  }

  batteryTrend.lastVoltageMv = voltageMv;
  batteryTrend.lastLevel = batteryLevel;
  batteryTrend.minVoltageMv = min(batteryTrend.minVoltageMv, voltageMv);
  batteryTrend.maxVoltageMv = max(batteryTrend.maxVoltageMv, voltageMv);

  const bool confirmedDropFromPeak =
      batteryTrend.fallingSamples >= CHARGING_CLEAR_SAMPLES &&
      batteryTrend.maxVoltageMv - batteryTrend.lastVoltageMv >
          CHARGING_TREND_THRESHOLD_MV;
  if (confirmedDropFromPeak) {
    batteryTrend.firstVoltageMv = voltageMv;
    batteryTrend.minVoltageMv = voltageMv;
    batteryTrend.maxVoltageMv = voltageMv;
    batteryTrend.firstLevel = batteryLevel;
    batteryTrend.risingSamples = 0;
    batteryTrend.fallingSamples = 0;
    batteryTrend.trendAboveThresholdSamples = 0;
    batteryTrend.inferredCharging = false;
    return;
  }

  const int voltageDelta = batteryTrend.lastVoltageMv - batteryTrend.minVoltageMv;

  if (voltageDelta > CHARGING_TREND_THRESHOLD_MV) {
    batteryTrend.trendAboveThresholdSamples++;
  } else {
    batteryTrend.trendAboveThresholdSamples = 0;
    batteryTrend.inferredCharging = false;
  }

  if (batteryTrend.trendAboveThresholdSamples >= CHARGING_CONFIRM_SAMPLES) {
    batteryTrend.inferredCharging = true;
  }
}

bool isExternalPowerPresent() {
  return lastVbusVoltageMv >= VBUS_PRESENT_THRESHOLD_MV;
}

bool isFilteredCharging() {
  return isExternalPowerPresent() && batteryTrend.initialized &&
         batteryTrend.inferredCharging;
}

bool isBatteryLevelDisplayable() {
  if (lastBatteryLevel < 0) {
    return false;
  }

  if (lastChargingStatus == m5::Power_Class::is_charging && !isFilteredCharging()) {
    return false;
  }

  return true;
}

const char* chargingStatusText(m5::Power_Class::is_charging_t status) {
  switch (status) {
    case m5::Power_Class::is_charging:
      return "Charging";
    case m5::Power_Class::is_discharging:
      return "Not charging";
    case m5::Power_Class::charge_unknown:
    default:
      return "Unknown";
  }
}

const char* inferredChargingText(m5::Power_Class::is_charging_t status) {
  (void)status;

  if (isFilteredCharging()) {
    return "Charging";
  }

  return "Not charging";
}
