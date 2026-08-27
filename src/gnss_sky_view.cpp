#include "app.h"

namespace {

String gnssSkyFixText() {
  if (loraGnssHasFreshFix()) {
    return "Fix";
  }

  return loraGnssCharsParsed > 0 ? "NoFix" : "Wait";
}

String gnssSkyAgeText() {
  if (lastGnssSkyGsvMs == 0) {
    return "wait";
  }

  return String((millis() - lastGnssSkyGsvMs) / 1000UL) + "s";
}

String gnssSkySatelliteLabel(const GnssSkySatellite& satellite) {
  char label[8];
  snprintf(label, sizeof(label), "%c%02u", satellite.constellation,
           satellite.prn);
  return label;
}

String gnssSkySatelliteSnrText(const GnssSkySatellite& satellite) {
  if (satellite.snrDb < 0) {
    return "--";
  }

  return String(satellite.snrDb);
}

uint16_t gnssSkySnrColor(const GnssSkySatellite& satellite) {
  if (satellite.snrDb < 0) {
    return DARKGREY;
  }
  if (satellite.snrDb >= 40) {
    return GREEN;
  }
  if (satellite.snrDb >= 25) {
    return YELLOW;
  }
  if (satellite.snrDb >= 10) {
    return ORANGE;
  }

  return RED;
}

const GnssSkySatellite* strongestGnssSkySatellite() {
  const GnssSkySatellite* strongest = nullptr;

  for (int i = 0; i < GNSS_SKY_MAX_SATELLITES; ++i) {
    const GnssSkySatellite& satellite = gnssSkySatellites[i];
    if (!satellite.active || satellite.snrDb < 0) {
      continue;
    }

    if (strongest == nullptr || satellite.snrDb > strongest->snrDb) {
      strongest = &satellite;
    }
  }

  return strongest;
}

void drawGnssSkyGrid(int centerX, int centerY, int radius) {
  contentCanvas.drawCircle(centerX, centerY, radius, DARKGREY);
  contentCanvas.drawCircle(centerX, centerY, (radius * 2) / 3, DARKGREY);
  contentCanvas.drawCircle(centerX, centerY, radius / 3, DARKGREY);
  contentCanvas.drawLine(centerX - radius, centerY, centerX + radius, centerY,
                         DARKGREY);
  contentCanvas.drawLine(centerX, centerY - radius, centerX, centerY + radius,
                         DARKGREY);

  contentCanvas.setTextColor(WHITE, BLACK);
  contentCanvas.setCursor(centerX - 3, centerY - radius - 1);
  contentCanvas.print("N");
  contentCanvas.setCursor(centerX + radius - 5, centerY - 6);
  contentCanvas.print("E");
  contentCanvas.setCursor(centerX - 3, centerY + radius - 10);
  contentCanvas.print("S");
  contentCanvas.setCursor(centerX - radius + 1, centerY - 6);
  contentCanvas.print("W");
}

void drawGnssSkySatellite(const GnssSkySatellite& satellite, int centerX,
                          int centerY, int radius) {
  const float elevation =
      constrain(static_cast<float>(satellite.elevationDeg), 0.0f, 90.0f);
  const float plotRadius = radius * ((90.0f - elevation) / 90.0f);
  const float azimuthRad =
      static_cast<float>(satellite.azimuthDeg) * PI / 180.0f;
  const int satX =
      centerX + static_cast<int>(roundf(sinf(azimuthRad) * plotRadius));
  const int satY =
      centerY - static_cast<int>(roundf(cosf(azimuthRad) * plotRadius));
  const uint16_t color = gnssSkySnrColor(satellite);

  contentCanvas.fillCircle(satX, satY, 3, color);
  contentCanvas.drawCircle(satX, satY, 4, WHITE);
}

void drawGnssSkyPlot() {
  refreshGnssSkySatellites();

  const int centerX = 54;
  const int centerY = contentCanvas.height() / 2;
  const int availableRadius = static_cast<int>(contentCanvas.height() / 2 - 7);
  const int radius = min(availableRadius, 48);

  drawGnssSkyGrid(centerX, centerY, radius);

  for (int i = 0; i < GNSS_SKY_MAX_SATELLITES; ++i) {
    if (gnssSkySatellites[i].active) {
      drawGnssSkySatellite(gnssSkySatellites[i], centerX, centerY, radius);
    }
  }

  if (gnssSkySatelliteCount == 0) {
    contentCanvas.setTextColor(DARKGREY, BLACK);
    contentCanvas.setCursor(centerX - 28, centerY - 6);
    contentCanvas.print("Waiting");
    contentCanvas.setTextColor(WHITE, BLACK);
  }
}

void drawGnssSkyReadout() {
  const int x = 112;
  int y = 0;
  const GnssSkySatellite* strongest = strongestGnssSkySatellite();

  contentCanvas.setTextColor(WHITE, BLACK);
  contentCanvas.setCursor(x, y);
  contentCanvas.printf("Sky:%lu/%lu", static_cast<unsigned long>(gnssSkySatelliteCount),
                       static_cast<unsigned long>(gnssSkySatellitesInView));

  y += 14;
  contentCanvas.setCursor(x, y);
  contentCanvas.printf("GSV:%lu %s", static_cast<unsigned long>(gnssSkyGsvSentenceCount),
                       gnssSkyAgeText().c_str());

  y += 14;
  contentCanvas.setCursor(x, y);
  contentCanvas.printf("Fix:%s S:%s", gnssSkyFixText().c_str(),
                       loraGnssSatellitesText().c_str());

  y += 14;
  contentCanvas.setCursor(x, y);
  contentCanvas.printf("HD:%s", loraGnssHdopText().c_str());

  y += 14;
  contentCanvas.setCursor(x, y);
  if (strongest != nullptr) {
    contentCanvas.printf("Best:%s %s", gnssSkySatelliteLabel(*strongest).c_str(),
                         gnssSkySatelliteSnrText(*strongest).c_str());
  } else {
    contentCanvas.print("Best: --");
  }

  y += 14;
  contentCanvas.setCursor(x, y);
  contentCanvas.print(loraGnssUtcText());

  y += 14;
  contentCanvas.setCursor(x, y);
  contentCanvas.print("R reset");
}

}  // namespace

void showGnssSkyView() {
  if (!gnssSkyViewInitialized) {
    initGnssSkyView();
  }

  serviceGnssSkyView();
  renderGnssSkyView();
}

void renderGnssSkyView() {
  lastGnssSkyViewRenderMs = millis();
  beginContentDraw();
  drawGnssSkyPlot();
  drawGnssSkyReadout();
  commitContentDraw();
}

bool initGnssSkyView() {
  resetGnssSkyView();
  gnssSkyViewInitialized = true;
  startLoraGnssSerial();
  lastGnssSkyViewServiceMs = 0;
  lastGnssSkyViewRenderMs = 0;
  Serial.println("GNSS sky view: started GSV satellite parser.");
  return true;
}

void serviceGnssSkyView() {
  if (!gnssSkyViewInitialized) {
    return;
  }

  const unsigned long now = millis();
  if (now - lastGnssSkyViewServiceMs < GNSS_SKY_SERVICE_INTERVAL_MS) {
    return;
  }

  lastGnssSkyViewServiceMs = now;
  serviceLoraGnssSerial();
  refreshGnssSkySatellites();

  if (currentScreen == Screen::GnssSkyView &&
      now - lastGnssSkyViewRenderMs >= GNSS_SKY_RENDER_INTERVAL_MS) {
    renderGnssSkyView();
  }
}

void resetGnssSkyView() {
  gnssSkyViewInitialized = false;
  resetLoraGnssParser();
  lastGnssSkyViewServiceMs = 0;
  lastGnssSkyViewRenderMs = 0;
}

void stopGnssSkyView() {
  stopLoraGnssSerial();
  resetGnssSkyView();
}
