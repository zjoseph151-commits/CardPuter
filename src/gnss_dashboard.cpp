#include "app.h"

namespace {

String clippedGnssDashboardText(const String& text, int maxChars) {
  return text.substring(0, maxChars);
}

String gnssFixStateText() {
  if (loraGnssHasFreshFix()) {
    return "Fix";
  }

  return loraGnssCharsParsed > 0 ? "NoFix" : "Wait";
}

String gnssDateTimeText() {
  if (!loraGnssTimeValid && !loraGnssDateValid) {
    return loraGnssStatus;
  }

  return loraGnssUtcText() + " " + loraGnssDateText().substring(5);
}

}  // namespace

void showGnssDashboard() {
  if (!gnssDashboardInitialized) {
    initGnssDashboard();
  }

  serviceGnssDashboard();
  renderGnssDashboard();
}

void renderGnssDashboard() {
  lastGnssDashboardRenderMs = millis();
  beginContentDraw();

  contentCanvas.printf("GNSS:%s Sat:%s HD:%s\n", gnssFixStateText().c_str(),
                       loraGnssSatellitesText().c_str(),
                       loraGnssHdopText().c_str());
  contentCanvas.println(clippedGnssDashboardText(
      loraGnssCoordinateText("Lat", loraGnssLatitude), 24));
  contentCanvas.println(clippedGnssDashboardText(
      loraGnssCoordinateText("Lon", loraGnssLongitude), 24));
  contentCanvas.printf("Speed:%s Alt:%s\n", loraGnssSpeedText().c_str(),
                       loraGnssAltitudeText().c_str());
  contentCanvas.println(clippedGnssDashboardText(gnssDateTimeText(), 24));
  contentCanvas.printf("Age:%lus Lines:%lu\n",
                       static_cast<unsigned long>(loraGnssFixAgeMs / 1000UL),
                       static_cast<unsigned long>(loraGnssLineCount));
  contentCanvas.printf("Chk OK:%lu Bad:%lu\n",
                       static_cast<unsigned long>(loraGnssPassedChecksum),
                       static_cast<unsigned long>(loraGnssFailedChecksum));
  contentCanvas.printf("R reset Bytes:%lu\n",
                       static_cast<unsigned long>(loraGnssByteCount));

  commitContentDraw();
}

bool initGnssDashboard() {
  resetGnssDashboard();
  gnssDashboardInitialized = true;
  startLoraGnssSerial();
  lastGnssDashboardServiceMs = 0;
  lastGnssDashboardRenderMs = 0;
  Serial.println("GNSS dashboard: started Cap LoRa-1262 GNSS parser.");
  return true;
}

void serviceGnssDashboard() {
  if (!gnssDashboardInitialized) {
    return;
  }

  const unsigned long now = millis();
  if (now - lastGnssDashboardServiceMs < GNSS_DASH_SERVICE_INTERVAL_MS) {
    return;
  }

  lastGnssDashboardServiceMs = now;
  serviceLoraGnssSerial();

  if (currentScreen == Screen::GnssDashboard &&
      now - lastGnssDashboardRenderMs >= GNSS_DASH_RENDER_INTERVAL_MS) {
    renderGnssDashboard();
  }
}

void resetGnssDashboard() {
  gnssDashboardInitialized = false;
  resetLoraGnssParser();
  lastGnssDashboardServiceMs = 0;
  lastGnssDashboardRenderMs = 0;
}

void stopGnssDashboard() {
  stopLoraGnssSerial();
  resetGnssDashboard();
}
