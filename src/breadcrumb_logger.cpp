#include "app.h"

namespace {

String clippedBreadcrumbText(const String& text, int maxChars) {
  return text.substring(0, maxChars);
}

bool breadcrumbHasValidCoordinate() {
  return isfinite(loraGnssLatitude) && isfinite(loraGnssLongitude) &&
         loraGnssLatitude >= -90.0 && loraGnssLatitude <= 90.0 &&
         loraGnssLongitude >= -180.0 && loraGnssLongitude <= 180.0;
}

bool breadcrumbHasFreshLogFix() {
  return loraGnssHasFreshFix() && breadcrumbHasValidCoordinate();
}

String breadcrumbFixText() {
  if (breadcrumbHasFreshLogFix()) {
    return "Fix";
  }

  return loraGnssCharsParsed > 0 ? "NoFix" : "Wait";
}

String breadcrumbUtcCsvText() {
  if (!loraGnssTimeValid) {
    return "";
  }

  char text[12];
  snprintf(text, sizeof(text), "%02u:%02u:%02u", loraGnssHour,
           loraGnssMinute, loraGnssSecond);
  return text;
}

String breadcrumbDateCsvText() {
  if (!loraGnssDateValid) {
    return "";
  }

  char text[12];
  snprintf(text, sizeof(text), "%04u-%02u-%02u", loraGnssYear,
           loraGnssMonth, loraGnssDay);
  return text;
}

}  // namespace

void showBreadcrumbLogger() {
  if (!breadcrumbLoggerInitialized) {
    initBreadcrumbLogger();
  }

  serviceBreadcrumbLogger();
  renderBreadcrumbLogger();
}

void renderBreadcrumbLogger() {
  lastBreadcrumbLoggerRenderMs = millis();
  beginContentDraw();

  contentCanvas.printf("GNSS:%s Sat:%s HD:%s\n", breadcrumbFixText().c_str(),
                       loraGnssSatellitesText().c_str(),
                       loraGnssHdopText().c_str());

  if (breadcrumbLogging) {
    contentCanvas.printf("Log:%s #%lu\n",
                         clippedBreadcrumbText(breadcrumbLogFileName, 15)
                             .c_str(),
                         static_cast<unsigned long>(breadcrumbLogSampleCount));
    contentCanvas.println("S stop CSV log");
  } else {
    contentCanvas.println("S start CSV log");
    if (breadcrumbLogFileName.length() > 0) {
      contentCanvas.printf("Last:%s #%lu\n",
                           clippedBreadcrumbText(breadcrumbLogFileName, 15)
                               .c_str(),
                           static_cast<unsigned long>(breadcrumbLogSampleCount));
    }
  }

  contentCanvas.println(clippedBreadcrumbText(breadcrumbLogStatus, 24));
  contentCanvas.println(clippedBreadcrumbText(
      loraGnssCoordinateText("Lat", loraGnssLatitude), 24));
  contentCanvas.println(clippedBreadcrumbText(
      loraGnssCoordinateText("Lon", loraGnssLongitude), 24));
  contentCanvas.println(clippedBreadcrumbText(loraGnssTimeText(), 24));
  contentCanvas.printf("Miss:%lu Lines:%lu\n",
                       static_cast<unsigned long>(breadcrumbLogMissedFixCount),
                       static_cast<unsigned long>(loraGnssLineCount));
  contentCanvas.println("OK/R reset");

  commitContentDraw();
}

bool initBreadcrumbLogger() {
  resetBreadcrumbLogger();
  breadcrumbLoggerInitialized = true;
  breadcrumbLogStatus = "S start CSV log";
  startLoraGnssSerial();
  lastBreadcrumbLoggerServiceMs = 0;
  lastBreadcrumbLoggerRenderMs = 0;
  Serial.println("Breadcrumb Logger: started GNSS parser. No LoRa radio.");
  return true;
}

void serviceBreadcrumbLogger() {
  if (!breadcrumbLoggerInitialized) {
    return;
  }

  const unsigned long now = millis();
  if (now - lastBreadcrumbLoggerServiceMs < BREADCRUMB_LOGGER_SERVICE_INTERVAL_MS) {
    return;
  }

  lastBreadcrumbLoggerServiceMs = now;
  serviceLoraGnssSerial();
  appendBreadcrumbLogSample();

  if (currentScreen == Screen::BreadcrumbLogger &&
      now - lastBreadcrumbLoggerRenderMs >= BREADCRUMB_LOGGER_RENDER_INTERVAL_MS) {
    renderBreadcrumbLogger();
  }
}

void resetBreadcrumbLogger() {
  if (breadcrumbLogFile) {
    breadcrumbLogFile.flush();
    breadcrumbLogFile.close();
  }

  breadcrumbLoggerInitialized = false;
  breadcrumbLogging = false;
  breadcrumbLogStatus = "Not started.";
  breadcrumbLogFileName = "";
  breadcrumbLogFilePath = "";
  breadcrumbLogSampleCount = 0;
  breadcrumbLogMissedFixCount = 0;
  lastBreadcrumbLoggerServiceMs = 0;
  lastBreadcrumbLoggerRenderMs = 0;
  lastBreadcrumbLogSampleMs = 0;
  resetLoraGnssParser();
}

void stopBreadcrumbLogger() {
  if (breadcrumbLogging) {
    stopBreadcrumbLogging("Log stopped.");
  }

  stopLoraGnssSerial();
  resetBreadcrumbLogger();
}

bool initBreadcrumbLogSd() {
  prepareSharedSpiForSd();

  if (!SD.begin(SD_SPI_CS_PIN, SPI, SD_SPI_FREQUENCY)) {
    sharedSpiOwner = SHARED_SPI_OWNER_NONE;
    breadcrumbLogStatus = "SD init failed.";
    Serial.println("Breadcrumb Logger: SD init failed.");
    return false;
  }

  if (SD.cardType() == CARD_NONE) {
    breadcrumbLogStatus = "No SD card.";
    Serial.println("Breadcrumb Logger: no SD card.");
    return false;
  }

  if (!SD.exists(BREADCRUMB_LOG_DIR) && !SD.mkdir(BREADCRUMB_LOG_DIR)) {
    breadcrumbLogStatus = "Cannot make /tracks.";
    Serial.println("Breadcrumb Logger: could not create /tracks.");
    return false;
  }

  return true;
}

bool findNextBreadcrumbLogPath(String& path, String& name) {
  if (!initBreadcrumbLogSd()) {
    return false;
  }

  for (int i = 1; i <= 999; ++i) {
    char filename[24];
    snprintf(filename, sizeof(filename), "track%03d.csv", i);
    name = filename;
    path = String(BREADCRUMB_LOG_DIR) + "/" + name;

    if (!SD.exists(path.c_str())) {
      return true;
    }
  }

  breadcrumbLogStatus = "Track list full.";
  return false;
}

bool startBreadcrumbLogging() {
  if (breadcrumbLogging) {
    return true;
  }

  if (!findNextBreadcrumbLogPath(breadcrumbLogFilePath, breadcrumbLogFileName)) {
    return false;
  }

  breadcrumbLogFile = SD.open(breadcrumbLogFilePath.c_str(), FILE_WRITE);
  if (!breadcrumbLogFile) {
    breadcrumbLogStatus = "Track open failed.";
    Serial.printf("Breadcrumb Logger: open failed for %s\n",
                  breadcrumbLogFilePath.c_str());
    return false;
  }

  breadcrumbLogFile.println(BREADCRUMB_LOG_HEADER);
  breadcrumbLogFile.flush();
  breadcrumbLogSampleCount = 0;
  breadcrumbLogMissedFixCount = 0;
  lastBreadcrumbLogSampleMs = 0;
  breadcrumbLogging = true;
  breadcrumbLogStatus =
      breadcrumbHasFreshLogFix() ? "Logging CSV." : "Logging wait fix.";
  Serial.printf("Breadcrumb Logger: started %s\n",
                breadcrumbLogFilePath.c_str());
  return true;
}

void stopBreadcrumbLogging(const char* message) {
  if (!breadcrumbLogging) {
    return;
  }

  breadcrumbLogging = false;
  if (breadcrumbLogFile) {
    breadcrumbLogFile.flush();
    breadcrumbLogFile.close();
  }

  breadcrumbLogStatus = message;
  Serial.printf("Breadcrumb Logger: stopped %s (%lu samples, %lu missed).\n",
                breadcrumbLogFilePath.c_str(),
                static_cast<unsigned long>(breadcrumbLogSampleCount),
                static_cast<unsigned long>(breadcrumbLogMissedFixCount));
}

void toggleBreadcrumbLogging() {
  if (breadcrumbLogging) {
    stopBreadcrumbLogging("Log stopped.");
  } else {
    startBreadcrumbLogging();
  }
}

void appendBreadcrumbLogSample() {
  if (!breadcrumbLogging || !breadcrumbLogFile) {
    return;
  }

  const unsigned long now = millis();
  if (lastBreadcrumbLogSampleMs > 0 &&
      now - lastBreadcrumbLogSampleMs < BREADCRUMB_LOG_SAMPLE_INTERVAL_MS) {
    return;
  }

  lastBreadcrumbLogSampleMs = now;

  if (!breadcrumbHasFreshLogFix()) {
    breadcrumbLogMissedFixCount++;
    breadcrumbLogStatus = "Waiting fresh fix.";
    return;
  }

  breadcrumbLogFile.print(now / 1000UL);
  breadcrumbLogFile.print(',');
  breadcrumbLogFile.print(breadcrumbUtcCsvText());
  breadcrumbLogFile.print(',');
  breadcrumbLogFile.print(breadcrumbDateCsvText());
  breadcrumbLogFile.print(',');
  breadcrumbLogFile.print(loraGnssLatitude, 6);
  breadcrumbLogFile.print(',');
  breadcrumbLogFile.print(loraGnssLongitude, 6);
  breadcrumbLogFile.print(',');
  if (loraGnssSatellitesValid) {
    breadcrumbLogFile.print(loraGnssSatellites);
  }
  breadcrumbLogFile.print(',');
  if (loraGnssHdopValid) {
    breadcrumbLogFile.print(loraGnssHdop, 1);
  }
  breadcrumbLogFile.print(',');
  if (loraGnssSpeedValid) {
    breadcrumbLogFile.print(loraGnssSpeedKmph, 1);
  }
  breadcrumbLogFile.print(',');
  if (loraGnssAltitudeValid) {
    breadcrumbLogFile.print(loraGnssAltitudeMeters, 1);
  }
  breadcrumbLogFile.println();
  breadcrumbLogFile.flush();

  breadcrumbLogSampleCount++;
  breadcrumbLogStatus = "Sample saved.";
  Serial.printf("Breadcrumb Logger: sample #%lu %.6f, %.6f.\n",
                static_cast<unsigned long>(breadcrumbLogSampleCount),
                loraGnssLatitude, loraGnssLongitude);
}
