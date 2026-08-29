#include "app.h"

namespace {

constexpr double RETURN_HOME_EARTH_RADIUS_METERS = 6371000.0;

String clippedReturnHomeText(const String& text, int maxChars) {
  return text.substring(0, maxChars);
}

String returnHomeFixText() {
  if (loraGnssHasFreshFix()) {
    return "Fix";
  }

  return loraGnssCharsParsed > 0 ? "NoFix" : "Wait";
}

bool validReturnHomeCoordinate(double latitude, double longitude) {
  return isfinite(latitude) && isfinite(longitude) && latitude >= -90.0 &&
         latitude <= 90.0 && longitude >= -180.0 && longitude <= 180.0;
}

double returnHomeRadians(double degrees) {
  return degrees * PI / 180.0;
}

double returnHomeDegrees(double radians) {
  return radians * 180.0 / PI;
}

double normalizedReturnHomeDegrees(double degrees) {
  while (degrees < 0.0) {
    degrees += 360.0;
  }
  while (degrees >= 360.0) {
    degrees -= 360.0;
  }
  return degrees;
}

float returnHomeDistanceBetween(double fromLatitude, double fromLongitude,
                                double toLatitude, double toLongitude) {
  const double fromLatRad = returnHomeRadians(fromLatitude);
  const double toLatRad = returnHomeRadians(toLatitude);
  const double deltaLatRad = returnHomeRadians(toLatitude - fromLatitude);
  const double deltaLonRad = returnHomeRadians(toLongitude - fromLongitude);
  const double sinHalfLat = sin(deltaLatRad / 2.0);
  const double sinHalfLon = sin(deltaLonRad / 2.0);
  const double haversine = sinHalfLat * sinHalfLat +
                           cos(fromLatRad) * cos(toLatRad) * sinHalfLon *
                               sinHalfLon;
  const double clampedHaversine = constrain(haversine, 0.0, 1.0);
  const double arc =
      2.0 * atan2(sqrt(clampedHaversine), sqrt(1.0 - clampedHaversine));

  return static_cast<float>(RETURN_HOME_EARTH_RADIUS_METERS * arc);
}

float returnHomeBearingBetween(double fromLatitude, double fromLongitude,
                               double toLatitude, double toLongitude) {
  const double fromLatRad = returnHomeRadians(fromLatitude);
  const double toLatRad = returnHomeRadians(toLatitude);
  const double deltaLonRad = returnHomeRadians(toLongitude - fromLongitude);
  const double y = sin(deltaLonRad) * cos(toLatRad);
  const double x = cos(fromLatRad) * sin(toLatRad) -
                   sin(fromLatRad) * cos(toLatRad) * cos(deltaLonRad);

  return static_cast<float>(
      normalizedReturnHomeDegrees(returnHomeDegrees(atan2(y, x))));
}

String returnHomeCoordinatePairText(const char* label, double latitude,
                                    double longitude) {
  if (!validReturnHomeCoordinate(latitude, longitude)) {
    return String(label) + ": --";
  }

  char text[32];
  snprintf(text, sizeof(text), "%s:%+.4f,%+.4f", label, latitude, longitude);
  return text;
}

}  // namespace

void showReturnHome() {
  if (!returnHomeInitialized) {
    initReturnHome();
  }

  serviceReturnHome();
  renderReturnHome();
}

void renderReturnHome() {
  lastReturnHomeRenderMs = millis();
  beginContentDraw();

  contentCanvas.printf("GNSS:%s Sat:%s HD:%s\n", returnHomeFixText().c_str(),
                       loraGnssSatellitesText().c_str(),
                       loraGnssHdopText().c_str());

  if (!returnHomeWaypointValid) {
    contentCanvas.println("Home: not saved");
    contentCanvas.println(loraGnssHasFreshFix() ? "S save current fix"
                                                : "Need fresh GNSS fix");
    contentCanvas.println(clippedReturnHomeText(returnHomeCoordinatePairText(
                                                    "Now", loraGnssLatitude,
                                                    loraGnssLongitude),
                                                24));
    contentCanvas.println(clippedReturnHomeText(loraGnssTimeText(), 24));
    contentCanvas.printf("Lines:%lu Bad:%lu\n",
                         static_cast<unsigned long>(loraGnssLineCount),
                         static_cast<unsigned long>(loraGnssFailedChecksum));
    contentCanvas.println("OK/R reset");
    commitContentDraw();
    return;
  }

  contentCanvas.printf("Dist:%s Brg:%s\n", returnHomeDistanceText().c_str(),
                       returnHomeBearingText().c_str());
  contentCanvas.println(clippedReturnHomeText(returnHomeStatus, 24));
  contentCanvas.println(clippedReturnHomeText(returnHomeCoordinatePairText(
                                                  "Now", loraGnssLatitude,
                                                  loraGnssLongitude),
                                              24));
  contentCanvas.println(clippedReturnHomeText(returnHomeCoordinatePairText(
                                                  "Home", returnHomeLatitude,
                                                  returnHomeLongitude),
                                              24));
  contentCanvas.printf("Age:%lus Lines:%lu\n",
                       static_cast<unsigned long>(loraGnssFixAgeMs / 1000UL),
                       static_cast<unsigned long>(loraGnssLineCount));
  contentCanvas.println("S update D clear");
  contentCanvas.println("OK/R reset");

  commitContentDraw();
}

bool initReturnHome() {
  resetReturnHome();
  returnHomeInitialized = true;
  loadReturnHomeWaypoint();
  startLoraGnssSerial();
  lastReturnHomeServiceMs = 0;
  lastReturnHomeRenderMs = 0;
  Serial.println("Return Home: started GNSS parser. No transmit path.");
  return true;
}

void serviceReturnHome() {
  if (!returnHomeInitialized) {
    return;
  }

  const unsigned long now = millis();
  if (now - lastReturnHomeServiceMs < RETURN_HOME_SERVICE_INTERVAL_MS) {
    return;
  }

  lastReturnHomeServiceMs = now;
  serviceLoraGnssSerial();
  updateReturnHomeNavigation();

  if (currentScreen == Screen::ReturnHome &&
      now - lastReturnHomeRenderMs >= RETURN_HOME_RENDER_INTERVAL_MS) {
    renderReturnHome();
  }
}

void resetReturnHome() {
  returnHomeInitialized = false;
  returnHomeWaypointValid = false;
  returnHomeNavigationValid = false;
  returnHomeLatitude = 0.0;
  returnHomeLongitude = 0.0;
  returnHomeDistanceMeters = 0.0f;
  returnHomeBearingDeg = 0.0f;
  returnHomeStatus = "Not started.";
  resetLoraGnssParser();
  lastReturnHomeServiceMs = 0;
  lastReturnHomeRenderMs = 0;
}

void stopReturnHome() {
  stopLoraGnssSerial();
  resetReturnHome();
}

bool loadReturnHomeWaypoint() {
  returnHomeWaypointValid = false;
  returnHomeNavigationValid = false;

  Preferences prefs;
  if (!prefs.begin(RETURN_HOME_PREF_NAMESPACE, true)) {
    returnHomeStatus = "Home store failed";
    Serial.println("Return Home: could not open waypoint preferences.");
    return false;
  }

  const bool stored = prefs.getBool("valid", false);
  const double storedLatitude = prefs.getDouble("lat", 0.0);
  const double storedLongitude = prefs.getDouble("lon", 0.0);
  prefs.end();

  if (!stored) {
    returnHomeStatus = "Press S save home";
    return false;
  }

  if (!validReturnHomeCoordinate(storedLatitude, storedLongitude)) {
    returnHomeStatus = "Saved home invalid";
    Serial.println("Return Home: saved waypoint coordinates are invalid.");
    return false;
  }

  returnHomeLatitude = storedLatitude;
  returnHomeLongitude = storedLongitude;
  returnHomeWaypointValid = true;
  returnHomeStatus = "Home loaded";
  updateReturnHomeNavigation();
  Serial.printf("Return Home: loaded waypoint %.6f, %.6f.\n",
                returnHomeLatitude, returnHomeLongitude);
  return true;
}

bool saveReturnHomeWaypoint() {
  serviceLoraGnssSerial();

  if (!loraGnssHasFreshFix() ||
      !validReturnHomeCoordinate(loraGnssLatitude, loraGnssLongitude)) {
    returnHomeStatus = "Need fresh GNSS fix";
    Serial.println("Return Home: save skipped; no fresh GNSS fix.");
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(RETURN_HOME_PREF_NAMESPACE, false)) {
    returnHomeStatus = "Home save failed";
    Serial.println("Return Home: could not open waypoint preferences for write.");
    return false;
  }

  size_t written = 0;
  written += prefs.putDouble("lat", loraGnssLatitude);
  written += prefs.putDouble("lon", loraGnssLongitude);
  written += prefs.putBool("valid", true);
  prefs.end();

  if (written == 0) {
    returnHomeStatus = "Home save failed";
    Serial.println("Return Home: waypoint preference write failed.");
    return false;
  }

  returnHomeLatitude = loraGnssLatitude;
  returnHomeLongitude = loraGnssLongitude;
  returnHomeWaypointValid = true;
  updateReturnHomeNavigation();
  returnHomeStatus = "Home saved";
  Serial.printf("Return Home: saved waypoint %.6f, %.6f.\n",
                returnHomeLatitude, returnHomeLongitude);
  return true;
}

bool clearReturnHomeWaypoint() {
  Preferences prefs;
  if (!prefs.begin(RETURN_HOME_PREF_NAMESPACE, false)) {
    returnHomeStatus = "Home clear failed";
    Serial.println("Return Home: could not open waypoint preferences to clear.");
    return false;
  }

  const bool cleared = prefs.clear();
  prefs.end();

  returnHomeWaypointValid = false;
  returnHomeNavigationValid = false;
  returnHomeLatitude = 0.0;
  returnHomeLongitude = 0.0;
  returnHomeDistanceMeters = 0.0f;
  returnHomeBearingDeg = 0.0f;
  returnHomeStatus = cleared ? "Home cleared" : "Home clear failed";
  Serial.println(cleared ? "Return Home: waypoint cleared."
                         : "Return Home: waypoint clear failed.");
  return cleared;
}

void updateReturnHomeNavigation() {
  if (!returnHomeWaypointValid) {
    returnHomeNavigationValid = false;
    if (returnHomeStatus == "Not started.") {
      returnHomeStatus = "Press S save home";
    }
    return;
  }

  if (!loraGnssHasFreshFix() ||
      !validReturnHomeCoordinate(loraGnssLatitude, loraGnssLongitude)) {
    returnHomeNavigationValid = false;
    returnHomeStatus =
        loraGnssCharsParsed > 0 ? "Waiting fresh fix" : "Waiting NMEA";
    return;
  }

  returnHomeDistanceMeters =
      returnHomeDistanceBetween(loraGnssLatitude, loraGnssLongitude,
                                returnHomeLatitude, returnHomeLongitude);
  returnHomeBearingDeg =
      returnHomeBearingBetween(loraGnssLatitude, loraGnssLongitude,
                               returnHomeLatitude, returnHomeLongitude);
  returnHomeNavigationValid = true;
  returnHomeStatus =
      returnHomeDistanceMeters <= RETURN_HOME_ARRIVAL_RADIUS_METERS
          ? "At home point"
          : "Bearing home ready";
}

String returnHomeDistanceText() {
  if (!returnHomeNavigationValid) {
    return "--";
  }

  char text[14];
  if (returnHomeDistanceMeters < 1000.0f) {
    snprintf(text, sizeof(text), "%.0fm", returnHomeDistanceMeters);
  } else if (returnHomeDistanceMeters < 100000.0f) {
    snprintf(text, sizeof(text), "%.2fkm", returnHomeDistanceMeters / 1000.0f);
  } else {
    snprintf(text, sizeof(text), "%.1fkm", returnHomeDistanceMeters / 1000.0f);
  }
  return text;
}

String returnHomeBearingText() {
  if (!returnHomeNavigationValid) {
    return "--";
  }

  const uint16_t bearing =
      static_cast<uint16_t>(roundf(returnHomeBearingDeg)) % 360U;
  const String direction = gnssSkyCompassDirection(bearing);
  char text[16];
  snprintf(text, sizeof(text), "%03u %s", bearing, direction.c_str());
  return text;
}
