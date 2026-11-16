#include "app.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <sys/time.h>

#include "adsb_lol_ca.h"

namespace {

constexpr uint16_t SEARCH_RADIUS_NM = 25;
constexpr size_t MAX_RESPONSE_BYTES = 65536;
constexpr size_t MAX_AIRCRAFT_JSON_BYTES = 3072;
constexpr size_t AIRCRAFT_DOCUMENT_BYTES = 1536;
constexpr uint32_t RESPONSE_IDLE_TIMEOUT_MS = 20000;
constexpr uint8_t MAX_AIRCRAFT = 12;
constexpr uint8_t VISIBLE_AIRCRAFT = 3;
constexpr uint32_t REQUEST_COOLDOWN_MS = 60000;
constexpr uint32_t SNAPSHOT_STALE_SECONDS = 120;
constexpr float MAX_POSITION_AGE_SECONDS = 300.0f;
constexpr double RAD_PER_DEGREE = 0.017453292519943295;
constexpr double EARTH_RADIUS_NM = 3440.065;

struct Aircraft {
  char id[12] = {};
  char hex[12] = {};
  char type[8] = {};
  char registration[12] = {};
  float distanceNm = 0;
  float positionAgeSeconds = 0;
  float speedKnots = NAN;
  int32_t altitudeFeet = 0;
  uint16_t bearingDegrees = 0;
  bool altitudeKnown = false;
  bool onGround = false;
};

Aircraft aircraft[MAX_AIRCRAFT];
uint8_t aircraftCount = 0;
uint8_t selectedAircraft = 0;
bool detailView = false;
bool trackerActive = false;
bool loading = false;
bool hasSnapshot = false;
unsigned long lastRequestMs = 0;
unsigned long snapshotMs = 0;
unsigned long lastRenderMs = 0;
double searchLatitude = 0;
double searchLongitude = 0;
String trackerStatus = "R search nearby";

enum class ParseError {
  None,
  Invalid,
  Incomplete,
  EntryTooLarge,
  NoMemory,
};

// Frame one aircraft at a time; ArduinoJson validates each bounded object.
class BoundedJsonReader {
 public:
  BoundedJsonReader(NetworkClient& input, size_t length, size_t& received)
      : input_(input), total_(length), remaining_(length), received_(received) {
    received_ = 0;
  }

  int read() {
    if (pending_ >= 0) {
      const int value = pending_;
      pending_ = -1;
      return value;
    }
    if (offset_ == buffered_) {
      if (remaining_ == 0) return -1;
      const unsigned long startedMs = millis();
      for (;;) {
        const int available = input_.available();
        if (available > 0) {
          const size_t requested =
              min(remaining_, min(sizeof(buffer_), static_cast<size_t>(available)));
          const int received = input_.read(buffer_, requested);
          if (received > 0) {
            buffered_ = static_cast<size_t>(received);
            offset_ = 0;
            remaining_ -= buffered_;
            received_ += buffered_;
            break;
          }
        }
        if ((!input_.connected() && input_.available() <= 0) ||
            millis() - startedMs >= RESPONSE_IDLE_TIMEOUT_MS) {
          incomplete_ = true;
          Serial.printf("Flight Tracker: body stopped after %lu/%lu bytes\n",
                        static_cast<unsigned long>(total_ - remaining_),
                        static_cast<unsigned long>(total_));
          return -1;
        }
        delay(1);
      }
    }
    return buffer_[offset_++];
  }

  void unread(int value) { pending_ = value; }

  int nextNonWhitespace() {
    int value;
    do {
      value = read();
    } while (value == ' ' || value == '\r' || value == '\n' ||
             value == '\t');
    return value;
  }

  bool incomplete() const { return incomplete_; }

 private:
  NetworkClient& input_;
  size_t total_;
  size_t remaining_;
  size_t& received_;
  uint8_t buffer_[256] = {};
  size_t offset_ = 0;
  size_t buffered_ = 0;
  int pending_ = -1;
  bool incomplete_ = false;
};

bool skipJsonString(BoundedJsonReader& reader) {
  bool escaped = false;
  for (;;) {
    const int value = reader.read();
    if (value < 0) return false;
    if (escaped) {
      escaped = false;
    } else if (value == '\\') {
      escaped = true;
    } else if (value == '"') {
      return true;
    } else if (value < 32) {
      return false;
    }
  }
}

bool readJsonKey(BoundedJsonReader& reader, char key[16]) {
  size_t length = 0;
  bool escaped = false;
  for (;;) {
    const int value = reader.read();
    if (value < 0 || value < 32) return false;
    if (escaped) {
      escaped = false;
    } else if (value == '\\') {
      escaped = true;
      continue;
    } else if (value == '"') {
      key[length] = '\0';
      return true;
    }
    if (length + 1 < 16) key[length++] = static_cast<char>(value);
  }
}

bool skipJsonValue(BoundedJsonReader& reader) {
  const int first = reader.nextNonWhitespace();
  if (first == '"') return skipJsonString(reader);
  if (first == '{' || first == '[') {
    char closing[8] = {};
    uint8_t depth = 1;
    closing[0] = first == '{' ? '}' : ']';
    while (depth > 0) {
      const int value = reader.read();
      if (value < 0) return false;
      if (value == '"') {
        if (!skipJsonString(reader)) return false;
      } else if (value == '{' || value == '[') {
        if (depth >= sizeof(closing)) return false;
        closing[depth++] = value == '{' ? '}' : ']';
      } else if (value == '}' || value == ']') {
        if (value != closing[--depth]) return false;
      }
    }
    return true;
  }
  if (first < 0 || first == ',' || first == '}' || first == ']') return false;
  for (;;) {
    const int value = reader.read();
    if (value < 0) return true;
    if (value == ',' || value == '}' || value == ']' || value == ' ' ||
        value == '\r' || value == '\n' || value == '\t') {
      reader.unread(value);
      return true;
    }
  }
}

void copyAscii(char* destination, size_t capacity, const char* source) {
  if (capacity == 0) return;
  size_t length = 0;
  if (source != nullptr) {
    while (*source != '\0' && length + 1 < capacity) {
      const unsigned char c = static_cast<unsigned char>(*source++);
      if (c >= 33 && c <= 126) destination[length++] = c;
    }
  }
  destination[length] = '\0';
}

int64_t daysFromCivil(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yearOfEra = static_cast<unsigned>(year - era * 400);
  const unsigned dayOfYear =
      (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
  const unsigned yearOfEraDay = yearOfEra * 365 + yearOfEra / 4 -
                                yearOfEra / 100 + dayOfYear;
  return static_cast<int64_t>(era) * 146097 + yearOfEraDay - 719468;
}

bool gnssUtc(time_t& utc) {
  if (!loraGnssDateValid || !loraGnssTimeValid || loraGnssYear < 2024 ||
      loraGnssYear > 2099 || loraGnssMonth < 1 || loraGnssMonth > 12 ||
      loraGnssDay < 1 || loraGnssDay > 31 || loraGnssHour > 23 ||
      loraGnssMinute > 59 || loraGnssSecond > 60) {
    return false;
  }
  utc = static_cast<time_t>(
      daysFromCivil(loraGnssYear, loraGnssMonth, loraGnssDay) * 86400 +
      loraGnssHour * 3600 + loraGnssMinute * 60 + loraGnssSecond);
  return utc > 1700000000;
}

bool validCenter() {
  return loraGnssHasFreshFix() && isfinite(loraGnssLatitude) &&
         isfinite(loraGnssLongitude) && fabs(loraGnssLatitude) <= 90.0 &&
         fabs(loraGnssLongitude) <= 180.0;
}

void distanceAndBearing(double latitude, double longitude, float& distanceNm,
                        uint16_t& bearingDegrees) {
  const double lat1 = searchLatitude * RAD_PER_DEGREE;
  const double lat2 = latitude * RAD_PER_DEGREE;
  const double deltaLat = (latitude - searchLatitude) * RAD_PER_DEGREE;
  const double deltaLon = (longitude - searchLongitude) * RAD_PER_DEGREE;
  const double a = pow(sin(deltaLat / 2.0), 2) +
                   cos(lat1) * cos(lat2) * pow(sin(deltaLon / 2.0), 2);
  distanceNm = static_cast<float>(
      2.0 * EARTH_RADIUS_NM * asin(sqrt(constrain(a, 0.0, 1.0))));
  const double y = sin(deltaLon) * cos(lat2);
  const double x = cos(lat1) * sin(lat2) -
                   sin(lat1) * cos(lat2) * cos(deltaLon);
  const double degrees = atan2(y, x) / RAD_PER_DEGREE;
  bearingDegrees = static_cast<uint16_t>(
      static_cast<int>(lround(degrees + 360.0)) % 360);
}

void retainNearest(const Aircraft& candidate) {
  uint8_t insertAt = 0;
  while (insertAt < aircraftCount &&
         aircraft[insertAt].distanceNm <= candidate.distanceNm) {
    ++insertAt;
  }
  if (insertAt >= MAX_AIRCRAFT) return;
  const uint8_t newCount = min<uint8_t>(aircraftCount + 1, MAX_AIRCRAFT);
  for (int i = newCount - 1; i > insertAt; --i) {
    aircraft[i] = aircraft[i - 1];
  }
  aircraft[insertAt] = candidate;
  aircraftCount = newCount;
}

String ageText(const Aircraft& entry) {
  const uint32_t seconds = static_cast<uint32_t>(ceilf(
      entry.positionAgeSeconds +
      static_cast<float>(millis() - snapshotMs) / 1000.0f));
  if (seconds < 60) return String(seconds) + "s";
  return String(seconds / 60) + "m";
}

String altitudeText(const Aircraft& entry) {
  if (entry.onGround) return "GND";
  if (!entry.altitudeKnown) return "--ft";
  return String(entry.altitudeFeet) + "ft";
}

String speedText(const Aircraft& entry) {
  return isfinite(entry.speedKnots)
             ? String(static_cast<int>(lroundf(entry.speedKnots))) + "kt"
             : "--kt";
}

String effectiveStatus() {
  if (loading) return "Loading aircraft...";
  if (!validCenter()) return "Need fresh GNSS fix";
  if (WiFi.status() != WL_CONNECTED) return "WiFi offline; connect first";
  if (hasSnapshot && millis() - snapshotMs >=
                         SNAPSHOT_STALE_SECONDS * 1000UL) {
    return "Stale data; R refresh";
  }
  if (aircraftCount > 0) {
    bool allStale = true;
    for (uint8_t i = 0; i < aircraftCount; ++i) {
      if (aircraft[i].positionAgeSeconds +
              static_cast<float>(millis() - snapshotMs) / 1000.0f <=
          SNAPSHOT_STALE_SECONDS) {
        allStale = false;
        break;
      }
    }
    if (allStale) return "Stale positions; R refresh";
  }
  return trackerStatus;
}

void clearResults() {
  aircraftCount = 0;
  selectedAircraft = 0;
  detailView = false;
  hasSnapshot = false;
}

void parseAircraft(JsonObjectConst item, uint16_t& staleCount,
                   uint16_t& unpositionedCount) {
  JsonVariantConst latitudeValue = item["lat"];
  JsonVariantConst longitudeValue = item["lon"];
  JsonVariantConst ageValue = item["seen_pos"];
  if (latitudeValue.isNull() || longitudeValue.isNull() || ageValue.isNull()) {
    ++unpositionedCount;
    return;
  }
  const double latitude = latitudeValue.as<double>();
  const double longitude = longitudeValue.as<double>();
  const float positionAge = ageValue.as<float>();
  if (!isfinite(latitude) || !isfinite(longitude) ||
      fabs(latitude) > 90.0 || fabs(longitude) > 180.0 ||
      !isfinite(positionAge) || positionAge < 0.0f) {
    ++unpositionedCount;
    return;
  }
  if (positionAge > MAX_POSITION_AGE_SECONDS) {
    ++staleCount;
    return;
  }

  Aircraft candidate;
  copyAscii(candidate.hex, sizeof(candidate.hex), item["hex"] | "");
  copyAscii(candidate.id, sizeof(candidate.id), item["flight"] | "");
  if (candidate.id[0] == '\0') {
    copyAscii(candidate.id, sizeof(candidate.id), candidate.hex);
  }
  if (candidate.id[0] == '\0') return;
  copyAscii(candidate.type, sizeof(candidate.type), item["t"] | "");
  copyAscii(candidate.registration, sizeof(candidate.registration),
            item["r"] | "");
  candidate.positionAgeSeconds = positionAge;
  distanceAndBearing(latitude, longitude, candidate.distanceNm,
                     candidate.bearingDegrees);
  JsonVariantConst altitude = item["alt_baro"];
  if (altitude.is<const char*>()) {
    candidate.onGround = strcmp(altitude.as<const char*>(), "ground") == 0;
  } else if (!altitude.isNull()) {
    candidate.altitudeKnown = true;
    candidate.altitudeFeet = altitude.as<int32_t>();
  }
  JsonVariantConst speed = item["gs"];
  if (!speed.isNull()) {
    const float knots = speed.as<float>();
    if (isfinite(knots) && knots >= 0.0f && knots < 1500.0f) {
      candidate.speedKnots = knots;
    }
  }
  retainNearest(candidate);
}

ParseError readAircraftObject(BoundedJsonReader& reader,
                              uint16_t& staleCount,
                              uint16_t& unpositionedCount) {
  static char objectText[MAX_AIRCRAFT_JSON_BYTES + 1];
  size_t length = 0;
  int depth = 0;
  bool inString = false;
  bool escaped = false;
  int value = '{';
  do {
    if (length >= MAX_AIRCRAFT_JSON_BYTES) {
      return ParseError::EntryTooLarge;
    }
    objectText[length++] = static_cast<char>(value);
    if (inString) {
      if (escaped) escaped = false;
      else if (value == '\\') escaped = true;
      else if (value == '"') inString = false;
    } else if (value == '"') {
      inString = true;
    } else if (value == '{') {
      ++depth;
    } else if (value == '}') {
      --depth;
    }
    if (depth == 0) break;
    value = reader.read();
    if (value < 0) {
      return reader.incomplete() ? ParseError::Incomplete : ParseError::Invalid;
    }
  } while (true);
  objectText[length] = '\0';

  StaticJsonDocument<384> filter;
  for (const char* field : {"hex", "flight", "lat", "lon", "seen_pos",
                            "alt_baro", "gs", "t", "r"}) {
    filter[field] = true;
  }
  StaticJsonDocument<AIRCRAFT_DOCUMENT_BYTES> document;
  const DeserializationError error = deserializeJson(
      document, objectText, length, DeserializationOption::Filter(filter),
      DeserializationOption::NestingLimit(5));
  if (error == DeserializationError::NoMemory) return ParseError::NoMemory;
  if (error || !document.is<JsonObject>()) return ParseError::Invalid;
  parseAircraft(document.as<JsonObjectConst>(), staleCount, unpositionedCount);
  return ParseError::None;
}

ParseError parseApiResponse(NetworkClient& stream, size_t length,
                            size_t& receivedBytes,
                            uint16_t& apiAircraftCount, uint16_t& staleCount,
                            uint16_t& unpositionedCount) {
  BoundedJsonReader reader(stream, length, receivedBytes);
  if (reader.nextNonWhitespace() != '{') return ParseError::Invalid;
  bool sawAircraftArray = false;
  for (;;) {
    int value = reader.nextNonWhitespace();
    if (value == '}') break;
    if (value != '"') return ParseError::Invalid;
    char key[16];
    if (!readJsonKey(reader, key) || reader.nextNonWhitespace() != ':') {
      return ParseError::Invalid;
    }
    if (strcmp(key, "ac") == 0) {
      if (sawAircraftArray || reader.nextNonWhitespace() != '[') {
        return ParseError::Invalid;
      }
      sawAircraftArray = true;
      for (;;) {
        value = reader.nextNonWhitespace();
        if (value == ']') break;
        if (value != '{') return ParseError::Invalid;
        const ParseError error =
            readAircraftObject(reader, staleCount, unpositionedCount);
        if (error != ParseError::None) return error;
        ++apiAircraftCount;
        value = reader.nextNonWhitespace();
        if (value == ']') break;
        if (value != ',') return ParseError::Invalid;
      }
    } else if (!skipJsonValue(reader)) {
      return reader.incomplete() ? ParseError::Incomplete : ParseError::Invalid;
    }
    value = reader.nextNonWhitespace();
    if (value == '}') break;
    if (value != ',') return ParseError::Invalid;
  }
  if (!sawAircraftArray || reader.nextNonWhitespace() != -1) {
    return ParseError::Invalid;
  }
  return reader.incomplete() ? ParseError::Incomplete : ParseError::None;
}

}  // namespace

void showFlightTracker() {
  trackerActive = true;
  clearResults();
  trackerStatus = "R search nearby";
  startLoraGnssSerial();
  serviceLoraGnssSerial();
  renderFlightTracker();
}

void renderFlightTracker() {
  lastRenderMs = millis();
  beginContentDraw();
  contentCanvas.setFont(&fonts::Font0);
  contentCanvas.printf("adsb.lol ODbL %unm %u/%u\n", SEARCH_RADIUS_NM,
                       aircraftCount ? selectedAircraft + 1 : 0,
                       aircraftCount);
  contentCanvas.println(effectiveStatus().substring(0, 38));

  if (detailView && aircraftCount > 0) {
    const Aircraft& entry = aircraft[selectedAircraft];
    contentCanvas.printf("%s  #%s\n", entry.id, entry.hex);
    contentCanvas.printf("%.1fnm  %03u deg\n", entry.distanceNm,
                         entry.bearingDegrees);
    contentCanvas.printf("Alt %s  Spd %s\n", altitudeText(entry).c_str(),
                         speedText(entry).c_str());
    contentCanvas.printf("Pos age %s%s\n", ageText(entry).c_str(),
                         entry.positionAgeSeconds +
                                     static_cast<float>(millis() - snapshotMs) /
                                         1000.0f >
                                 SNAPSHOT_STALE_SECONDS
                             ? " STALE"
                             : "");
    contentCanvas.printf("Type %s  Reg %s\n",
                         entry.type[0] ? entry.type : "--",
                         entry.registration[0] ? entry.registration : "--");
    contentCanvas.println("From GNSS fix at fetch");
    contentCanvas.println("Arrows Back list R refresh");
    contentCanvas.println("Not for navigation");
  } else if (aircraftCount > 0) {
    const int first = (selectedAircraft / VISIBLE_AIRCRAFT) * VISIBLE_AIRCRAFT;
    for (int index = first;
         index < aircraftCount && index < first + VISIBLE_AIRCRAFT; ++index) {
      const Aircraft& entry = aircraft[index];
      contentCanvas.printf("%c%-10.10s %4.1fnm %03u\n",
                           index == selectedAircraft ? '>' : ' ', entry.id,
                           entry.distanceNm, entry.bearingDegrees);
      contentCanvas.printf("  %s  %s  age %s\n",
                           altitudeText(entry).c_str(),
                           speedText(entry).c_str(), ageText(entry).c_str());
    }
    contentCanvas.println("Arrows OK detail R refresh");
    contentCanvas.println("Back / Not for navigation");
  } else {
    contentCanvas.println("R search nearby aircraft");
    contentCanvas.println("Use WiFi Connect first");
    contentCanvas.println("GNSS fix sets the center");
    contentCanvas.println("R search  Back menu");
    contentCanvas.println("Not for navigation");
  }
  commitContentDraw();
  contentCanvas.setFont(&fonts::Font2);
}

void serviceFlightTracker() {
  if (!trackerActive) return;
  serviceLoraGnssSerial();
  if (millis() - lastRenderMs >= 1000) renderFlightTracker();
}

void stopFlightTracker() {
  stopLoraGnssSerial();
  trackerActive = false;
  loading = false;
}

void requestFlightTracker() {
  serviceLoraGnssSerial();
  if (!validCenter()) {
    trackerStatus = "Need fresh GNSS fix";
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    trackerStatus = "WiFi offline; connect first";
    return;
  }
  time_t utc = 0;
  if (!gnssUtc(utc)) {
    trackerStatus = "Need GNSS UTC for HTTPS";
    return;
  }
  const unsigned long nowMs = millis();
  if (lastRequestMs != 0 && nowMs - lastRequestMs < REQUEST_COOLDOWN_MS) {
    trackerStatus = "Wait 60s between searches";
    return;
  }

  searchLatitude = loraGnssLatitude;
  searchLongitude = loraGnssLongitude;
  clearResults();
  loading = true;
  trackerStatus = "Loading aircraft...";
  renderFlightTracker();
  renderOledStatusDashboard();

  timeval clock = {utc, 0};
  settimeofday(&clock, nullptr);
  WiFiClientSecure client;
  client.setCACert(ADSB_LOL_ROOT_CA);
  client.setTimeout(8000);
  HTTPClient http;
  const String url = String("https://api.adsb.lol/v2/lat/") +
                     String(searchLatitude, 5) + "/lon/" +
                     String(searchLongitude, 5) + "/dist/" +
                     SEARCH_RADIUS_NM;
  if (!http.begin(client, url)) {
    loading = false;
    trackerStatus = "HTTPS setup failed";
    return;
  }
  http.setTimeout(8000);
  http.addHeader("User-Agent", "Scoober-Cardputer/0.1");
  lastRequestMs = nowMs;
  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    trackerStatus = code == 429 ? "API rate limited (429)" :
                    code == 403 ? "API access denied (403)" :
                                  String("API HTTP error ") + code;
    http.end();
    loading = false;
    return;
  }
  const int length = http.getSize();
  if (length <= 0 || static_cast<size_t>(length) > MAX_RESPONSE_BYTES) {
    trackerStatus = length < 0 ? "API size unknown" : "API result too large";
    http.end();
    loading = false;
    return;
  }

  uint16_t apiAircraftCount = 0;
  uint16_t staleCount = 0;
  uint16_t unpositionedCount = 0;
  size_t receivedBytes = 0;
  const ParseError error =
      parseApiResponse(http.getStream(), static_cast<size_t>(length),
                       receivedBytes, apiAircraftCount, staleCount,
                       unpositionedCount);
  http.end();
  loading = false;
  if (error != ParseError::None) {
    clearResults();
    trackerStatus = error == ParseError::EntryTooLarge
                        ? "API aircraft entry too large"
                    : error == ParseError::NoMemory
                        ? "API aircraft fields too large"
                    : error == ParseError::Incomplete
                        ? String("API short ") + receivedBytes + "/" + length
                        : "API JSON invalid";
    return;
  }
  snapshotMs = millis();
  hasSnapshot = true;
  if (aircraftCount > 0) {
    trackerStatus = String("Nearby: ") + aircraftCount +
                    (apiAircraftCount > aircraftCount ? " nearest" : " aircraft");
  } else if (apiAircraftCount == 0) {
    trackerStatus = "No aircraft in range";
  } else if (staleCount > 0 && unpositionedCount == 0) {
    trackerStatus = "Only stale positions";
  } else {
    trackerStatus = "No recent positions";
  }
}

void moveFlightSelection(int direction) {
  if (aircraftCount == 0) return;
  const int next = constrain(static_cast<int>(selectedAircraft) + direction,
                             0, static_cast<int>(aircraftCount) - 1);
  selectedAircraft = static_cast<uint8_t>(next);
}

void openFlightDetail() {
  if (aircraftCount > 0) detailView = true;
}

bool closeFlightDetail() {
  if (!detailView) return false;
  detailView = false;
  return true;
}

void buildFlightTrackerOledLines(
    String lines[PI_MONITOR_OLED_MESSAGE_LINE_COUNT]) {
  lines[0] = "Flight Tracker";
  lines[1] = "adsb.lol / ODbL";
  lines[2] = detailView ? "Arrows: next aircraft" : "Arrows: select plane";
  lines[3] = detailView ? "Back: list" : "OK: aircraft detail";
  lines[4] = "R: search / refresh";
  lines[5] = "Back: menu from list";
  lines[6] = "Not for navigation";
  lines[7] = effectiveStatus();
}
