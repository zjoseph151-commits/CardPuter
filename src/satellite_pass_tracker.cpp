#include "app.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <sgp4pred.h>
#include <sys/time.h>

#include "celestrak_ca.h"

namespace {

struct SatelliteTarget {
  const char* name;
  uint32_t catalog;
};

constexpr SatelliteTarget BUILTIN_TARGETS[] = {
    {"ISS", 25544}, {"NOAA 18", 28654}, {"NOAA 19", 33591}};
constexpr uint8_t TARGET_COUNT = 4;
constexpr uint32_t DOWNLOAD_INTERVAL_SECONDS = 2UL * 60UL * 60UL;
constexpr uint32_t OLD_ELEMENTS_SECONDS = 3UL * 24UL * 60UL * 60UL;
constexpr uint32_t MAX_ELEMENTS_SECONDS = 14UL * 24UL * 60UL * 60UL;
constexpr unsigned long PREDICTION_INTERVAL_MS = 60000;
constexpr unsigned long RENDER_INTERVAL_MS = 1000;
constexpr double JULIAN_UNIX_EPOCH = 2440587.5;

Sgp4 orbit;
uint8_t selectedTarget = 0;
uint32_t customCatalog = 0;
String catalogDraft;
bool editingCatalog = false;
bool trackerInitialized = false;
bool elementsLoaded = false;
bool passAvailable = false;
bool hadObserver = false;
String elementName;
String elementLine1;
String elementLine2;
String trackerStatus;
uint32_t fetchedUnix = 0;
time_t riseUnix = 0;
time_t peakUnix = 0;
time_t setUnix = 0;
double riseAzimuth = 0.0;
double setAzimuth = 0.0;
double peakElevation = 0.0;
double elementAgeDays = NAN;
unsigned long lastPredictionMs = 0;
unsigned long lastRenderMs = 0;

uint32_t selectedCatalog() {
  return selectedTarget < 3 ? BUILTIN_TARGETS[selectedTarget].catalog
                            : customCatalog;
}

String selectedLabel() {
  if (selectedTarget < 3) return BUILTIN_TARGETS[selectedTarget].name;
  return customCatalog ? String("CAT ") + customCatalog : "Custom: unset";
}

String cachePath(uint32_t catalog) {
  return String("/config/sat") + catalog + ".txt";
}

bool validTleLine(const String& line, char lineNumber, uint32_t catalog) {
  if (line.length() != 69 || line.charAt(0) != lineNumber ||
      line.charAt(1) != ' ' ||
      static_cast<uint32_t>(line.substring(2, 7).toInt()) != catalog) {
    return false;
  }
  for (int i = 2; i < 7; ++i) {
    if (line.charAt(i) < '0' || line.charAt(i) > '9') return false;
  }
  int checksum = 0;
  for (int i = 0; i < 68; ++i) {
    const char c = line.charAt(i);
    if (c >= '0' && c <= '9') checksum += c - '0';
    else if (c == '-') ++checksum;
    else if (c < 32 || c > 126) return false;
  }
  return line.charAt(68) == '0' + (checksum % 10);
}

bool validTle(const String& line1, const String& line2, uint32_t catalog) {
  if (!validTleLine(line1, '1', catalog) ||
      !validTleLine(line2, '2', catalog)) {
    return false;
  }
  for (int i = 18; i < 32; ++i) {
    const char c = line1.charAt(i);
    if ((c < '0' || c > '9') && !(i == 23 && c == '.')) return false;
  }
  for (int i = 26; i < 33; ++i) {
    if (line2.charAt(i) < '0' || line2.charAt(i) > '9') return false;
  }
  const double inclination = line2.substring(8, 16).toDouble();
  const double meanMotion = line2.substring(52, 63).toDouble();
  return inclination >= 0.0 && inclination <= 180.0 &&
         meanMotion > 0.01 && meanMotion < 20.0;
}

// Gregorian civil date to days since 1970-01-01, avoiding local TZ conversion.
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

bool currentElementEpoch(const String& line1, time_t now) {
  const int shortYear = line1.substring(18, 20).toInt();
  const double dayOfYear = line1.substring(20, 32).toDouble();
  const int year = shortYear < 57 ? 2000 + shortYear : 1900 + shortYear;
  if (!isfinite(dayOfYear) || dayOfYear < 1.0 || dayOfYear > 366.999) {
    return false;
  }
  const double epoch =
      static_cast<double>(daysFromCivil(year, 1, 1) * 86400) +
      (dayOfYear - 1.0) * 86400.0;
  return epoch <= static_cast<double>(now) + 86400.0 &&
         epoch >= static_cast<double>(now) - MAX_ELEMENTS_SECONDS;
}

bool observerTime(time_t& now) {
  if (!loraGnssHasFreshFix() || !loraGnssDateValid ||
      !loraGnssTimeValid || loraGnssYear < 2024 ||
      loraGnssYear > 2099) {
    return false;
  }
  if (loraGnssMonth < 1 || loraGnssMonth > 12 ||
      loraGnssDay < 1 || loraGnssDay > 31 ||
      loraGnssHour > 23 || loraGnssMinute > 59 ||
      loraGnssSecond > 60) {
    return false;
  }
  now = static_cast<time_t>(
      daysFromCivil(loraGnssYear, loraGnssMonth, loraGnssDay) * 86400 +
      loraGnssHour * 3600 + loraGnssMinute * 60 + loraGnssSecond);
  return now > 1700000000;
}

String utcText(time_t value) {
  tm utc = {};
  gmtime_r(&value, &utc);
  char text[20];
  snprintf(text, sizeof(text), "%02d/%02d %02d:%02d", utc.tm_mon + 1,
           utc.tm_mday, utc.tm_hour, utc.tm_min);
  return text;
}

bool openCacheSd() {
  prepareSharedSpiForSd();
  if (!SD.begin(SD_SPI_CS_PIN, SPI, SD_SPI_FREQUENCY)) {
    sharedSpiOwner = SHARED_SPI_OWNER_NONE;
    trackerStatus = "SD unavailable";
    return false;
  }
  if (SD.cardType() == CARD_NONE) {
    trackerStatus = "Insert microSD";
    return false;
  }
  return true;
}

bool loadSelectedCache() {
  elementsLoaded = false;
  passAvailable = false;
  fetchedUnix = 0;
  elementAgeDays = NAN;
  elementName = "";
  elementLine1 = "";
  elementLine2 = "";
  lastPredictionMs = 0;
  const uint32_t catalog = selectedCatalog();
  if (catalog == 0) {
    trackerStatus = "Set custom catalog with N";
    return false;
  }
  if (!openCacheSd()) return false;
  const String path = cachePath(catalog);
  const String backup = path + ".bak";
  if (!SD.exists(path) && SD.exists(backup)) SD.rename(backup, path);
  File file = SD.open(path, FILE_READ);
  if (!file) {
    trackerStatus = "No cached elements; U fetch";
    return false;
  }
  if (file.size() > 512) {
    file.close();
    trackerStatus = "Cache too large";
    return false;
  }
  String first = file.readStringUntil('\n');
  first.trim();
  if (first.startsWith("fetched=")) {
    fetchedUnix = static_cast<uint32_t>(first.substring(8).toInt());
    elementName = file.readStringUntil('\n');
    elementName.trim();
    elementLine1 = file.readStringUntil('\n');
    elementLine2 = file.readStringUntil('\n');
  } else if (first.startsWith("1 ")) {
    elementName = selectedLabel();
    elementLine1 = first;
    elementLine2 = file.readStringUntil('\n');
  } else {
    elementName = first;
    elementLine1 = file.readStringUntil('\n');
    elementLine2 = file.readStringUntil('\n');
  }
  file.close();
  elementLine1.trim();
  elementLine2.trim();
  if (!validTle(elementLine1, elementLine2, catalog)) {
    trackerStatus = "Invalid cached TLE";
    return false;
  }
  elementsLoaded = true;
  trackerStatus = "Cached TLE loaded";
  return true;
}

bool parseDownloadedTle(String response, String& name, String& line1,
                        String& line2, uint32_t catalog) {
  response.replace("\r", "");
  int start = 0;
  while (start < response.length()) {
    int end = response.indexOf('\n', start);
    if (end < 0) end = response.length();
    String line = response.substring(start, end);
    line.trim();
    if (line.startsWith("1 ")) line1 = line;
    else if (line.startsWith("2 ")) line2 = line;
    else if (!line.isEmpty() && name.isEmpty()) name = line;
    start = end + 1;
  }
  return validTle(line1, line2, catalog);
}

bool saveSelectedCache(const String& name, const String& line1,
                       const String& line2, time_t now) {
  if (!openCacheSd()) return false;
  if (!SD.exists("/config") && !SD.mkdir("/config")) {
    trackerStatus = "Cannot create /config";
    return false;
  }
  const String path = cachePath(selectedCatalog());
  const String temporary = path + ".new";
  const String backup = path + ".bak";
  SD.remove(temporary);
  File file = SD.open(temporary, FILE_WRITE);
  if (!file) {
    trackerStatus = "Cache write failed";
    return false;
  }
  const String data = String("fetched=") +
                      String(static_cast<unsigned long>(now)) + "\n" +
                      name + "\n" + line1 + "\n" + line2 + "\n";
  const size_t written = file.print(data);
  file.flush();
  file.close();
  if (written != data.length()) {
    SD.remove(temporary);
    trackerStatus = "Cache write incomplete";
    return false;
  }
  SD.remove(backup);
  const bool hadCache = SD.exists(path);
  if (hadCache && !SD.rename(path, backup)) {
    SD.remove(temporary);
    trackerStatus = "Cache backup failed";
    return false;
  }
  if (!SD.rename(temporary, path)) {
    if (hadCache) SD.rename(backup, path);
    trackerStatus = "Cache replace failed";
    return false;
  }
  SD.remove(backup);
  return loadSelectedCache();
}

void calculatePass() {
  lastPredictionMs = millis();
  passAvailable = false;
  time_t now = 0;
  if (!observerTime(now)) {
    trackerStatus = "Need fresh GNSS fix + UTC";
    return;
  }
  if (!elementsLoaded) {
    trackerStatus = "Need cached TLE; U fetch";
    return;
  }
  char line1[130] = {};
  char line2[130] = {};
  strncpy(line1, elementLine1.c_str(), sizeof(line1) - 1);
  strncpy(line2, elementLine2.c_str(), sizeof(line2) - 1);
  if (strcmp(orbit.line1, elementLine1.c_str()) != 0 ||
      strcmp(orbit.line2, elementLine2.c_str()) != 0) {
    orbit.line1[0] = '\0';
    if (!orbit.init(selectedLabel().c_str(), line1, line2) ||
        orbit.satrec.error != 0 || !isfinite(orbit.revpday)) {
      trackerStatus = "SGP4 init failed";
      return;
    }
  }
  const double nowJd = JULIAN_UNIX_EPOCH +
                       static_cast<double>(now) / 86400.0;
  elementAgeDays = nowJd - orbit.satrec.jdsatepoch;
  if (!isfinite(elementAgeDays) || elementAgeDays < -1.0 ||
      elementAgeDays > 14.0) {
    trackerStatus = "TLE stale or clock wrong";
    return;
  }
  orbit.site(loraGnssLatitude, loraGnssLongitude,
             loraGnssAltitudeValid ? loraGnssAltitudeMeters : 0.0);
  if (!orbit.initpredpoint(static_cast<unsigned long>(now), 0.0)) {
    trackerStatus = "Pass search failed";
    return;
  }
  passinfo pass = {};
  bool futurePass = false;
  for (int attempt = 0; attempt < 3; ++attempt) {
    if (!orbit.nextpass(&pass, 32, false, 10.0) ||
        !isfinite(pass.jdstart) || !isfinite(pass.jdmax) ||
        !isfinite(pass.jdstop) || pass.maxelevation < 10.0) {
      trackerStatus = "No 10 deg pass found";
      return;
    }
    if (pass.jdmax >= nowJd) {
      futurePass = true;
      break;
    }
  }
  if (!futurePass) {
    trackerStatus = "No near-term pass";
    return;
  }
  riseUnix = static_cast<time_t>(llround(
      (pass.jdstart - JULIAN_UNIX_EPOCH) * 86400.0));
  peakUnix = static_cast<time_t>(llround(
      (pass.jdmax - JULIAN_UNIX_EPOCH) * 86400.0));
  setUnix = static_cast<time_t>(llround(
      (pass.jdstop - JULIAN_UNIX_EPOCH) * 86400.0));
  if (peakUnix < now || riseUnix > now + 2 * 86400 ||
      !(riseUnix < peakUnix && peakUnix < setUnix)) {
    trackerStatus = "No near-term pass";
    return;
  }
  riseAzimuth = pass.azstart;
  setAzimuth = pass.azstop;
  peakElevation = pass.maxelevation;
  passAvailable = true;
  trackerStatus = elementAgeDays > OLD_ELEMENTS_SECONDS / 86400.0
                      ? "Pass; old elements"
                      : "Pass ready";
}

}  // namespace

void showSatellitePassTracker() {
  if (!trackerInitialized) {
    trackerInitialized = true;
    Preferences prefs;
    if (prefs.begin("satpasses", true)) {
      customCatalog = prefs.getUInt("custom", 0);
      prefs.end();
    }
    startLoraGnssSerial();
    loadSelectedCache();
  }
  serviceSatellitePassTracker();
  renderSatellitePassTracker();
}

void renderSatellitePassTracker() {
  lastRenderMs = millis();
  beginContentDraw();
  contentCanvas.setFont(&fonts::Font0);
  contentCanvas.printf("%u/%u %s  #%lu\n", selectedTarget + 1, TARGET_COUNT,
                       selectedLabel().c_str(),
                       static_cast<unsigned long>(selectedCatalog()));
  contentCanvas.println(trackerStatus.substring(0, 36));
  if (editingCatalog) {
    contentCanvas.println("Catalog number (1-69999):");
    contentCanvas.printf("> %s\n", catalogDraft.c_str());
    contentCanvas.println("OK save  Del erase/cancel");
  } else {
    contentCanvas.printf("GNSS:%s UTC:%s\n",
                         loraGnssHasFreshFix() ? "fix" : "wait",
                         loraGnssTimeValid && loraGnssDateValid ? "ready"
                                                                 : "wait");
    contentCanvas.printf("TLE:%s Age:%s\n", elementsLoaded ? "SD" : "--",
                         isfinite(elementAgeDays)
                             ? (String(elementAgeDays, 1) + "d").c_str()
                             : "--");
    if (passAvailable) {
      contentCanvas.printf("Rise %s  Az %.0f\n", utcText(riseUnix).c_str(),
                           riseAzimuth);
      contentCanvas.printf("Peak %s  El %.0f\n", utcText(peakUnix).c_str(),
                           peakElevation);
      contentCanvas.printf("Set  %s  Az %.0f\n", utcText(setUnix).c_str(),
                           setAzimuth);
    } else {
      contentCanvas.println("Next pass: --");
    }
    contentCanvas.println("Arrows select  N custom");
    contentCanvas.println("U fetch TLE  OK/R recalc");
  }
  commitContentDraw();
  contentCanvas.setFont(&fonts::Font2);
}

void serviceSatellitePassTracker() {
  if (!trackerInitialized) return;
  serviceLoraGnssSerial();
  time_t now = 0;
  const bool observerReady = observerTime(now);
  if (!observerReady && passAvailable) {
    passAvailable = false;
    trackerStatus = "Need fresh GNSS fix + UTC";
  }
  if (observerReady && !hadObserver) lastPredictionMs = 0;
  hadObserver = observerReady;
  if (elementsLoaded && observerReady &&
      (lastPredictionMs == 0 ||
       millis() - lastPredictionMs >= PREDICTION_INTERVAL_MS ||
       (passAvailable && now >= setUnix))) {
    calculatePass();
  }
  if (millis() - lastRenderMs >= RENDER_INTERVAL_MS) {
    renderSatellitePassTracker();
  }
}

void stopSatellitePassTracker() {
  stopLoraGnssSerial();
  trackerInitialized = false;
  editingCatalog = false;
  catalogDraft = "";
  passAvailable = false;
  hadObserver = false;
}

void moveSatellitePassTarget(int direction) {
  const int next = constrain(static_cast<int>(selectedTarget) + direction,
                             0, static_cast<int>(TARGET_COUNT) - 1);
  if (next == selectedTarget) return;
  selectedTarget = static_cast<uint8_t>(next);
  elementAgeDays = NAN;
  loadSelectedCache();
  calculatePass();
}

void refreshSatellitePassElements() {
  const uint32_t catalog = selectedCatalog();
  if (catalog == 0) {
    trackerStatus = "Set custom catalog with N";
    return;
  }
  time_t now = 0;
  if (!observerTime(now)) {
    trackerStatus = "Need fresh GNSS + UTC";
    return;
  }
  if (fetchedUnix != 0 && now >= fetchedUnix &&
      static_cast<uint32_t>(now - fetchedUnix) < DOWNLOAD_INTERVAL_SECONDS) {
    trackerStatus = "TLE refresh: wait 2h";
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    trackerStatus = "Use WiFi Connect first";
    return;
  }
  const String attemptKey = String("a") + catalog;
  Preferences prefs;
  if (!prefs.begin("satpasses", false)) {
    trackerStatus = "TLE rate limit unavailable";
    return;
  }
  const uint32_t lastAttempt = prefs.getUInt(attemptKey.c_str(), 0);
  if (lastAttempt != 0 && now >= lastAttempt &&
      static_cast<uint32_t>(now - lastAttempt) < DOWNLOAD_INTERVAL_SECONDS) {
    prefs.end();
    trackerStatus = "TLE request: wait 2h";
    return;
  }
  timeval clock = {now, 0};
  settimeofday(&clock, nullptr);
  trackerStatus = "Downloading TLE...";
  renderSatellitePassTracker();
  WiFiClientSecure client;
  client.setCACert(CELESTRAK_ROOT_CA);
  client.setTimeout(8000);
  HTTPClient http;
  const String url = String("https://celestrak.org/NORAD/elements/gp.php?") +
                     "CATNR=" + catalog + "&FORMAT=TLE";
  if (!http.begin(client, url)) {
    prefs.end();
    trackerStatus = "HTTPS setup failed";
    return;
  }
  http.setTimeout(8000);
  http.addHeader("User-Agent", "Scoober-Cardputer/0.1");
  if (prefs.putUInt(attemptKey.c_str(), static_cast<uint32_t>(now)) == 0) {
    prefs.end();
    http.end();
    trackerStatus = "TLE rate limit save failed";
    return;
  }
  prefs.end();
  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    trackerStatus = String("TLE HTTP error ") + code;
    http.end();
    return;
  }
  const int length = http.getSize();
  if (length < 130 || length > 512) {
    trackerStatus = "TLE response size invalid";
    http.end();
    return;
  }
  const String response = http.getString();
  http.end();
  if (response.length() != length) {
    trackerStatus = "TLE response incomplete";
    return;
  }
  String name, line1, line2;
  if (!parseDownloadedTle(response, name, line1, line2, catalog)) {
    trackerStatus = "TLE checksum/ID invalid";
    return;
  }
  if (!currentElementEpoch(line1, now)) {
    trackerStatus = "Downloaded TLE is stale";
    return;
  }
  if (name.isEmpty()) name = selectedLabel();
  if (saveSelectedCache(name, line1, line2, now)) {
    calculatePass();
  }
}

void recalculateSatellitePass() {
  calculatePass();
}

void startSatelliteCatalogEntry() {
  editingCatalog = true;
  catalogDraft = "";
}

void appendSatelliteCatalogDigit(char digit) {
  if (editingCatalog && digit >= '0' && digit <= '9' &&
      catalogDraft.length() < 5) {
    catalogDraft += digit;
  }
}

void deleteSatelliteCatalogDigit() {
  if (!catalogDraft.isEmpty()) {
    catalogDraft.remove(catalogDraft.length() - 1);
  } else {
    editingCatalog = false;
  }
}

void applySatelliteCatalogEntry() {
  if (!editingCatalog) return;
  const uint32_t catalog = catalogDraft.toInt();
  if (catalog == 0 || catalog >= 70000) {
    trackerStatus = "Enter catalog 1-69999";
    return;
  }
  customCatalog = catalog;
  Preferences prefs;
  if (prefs.begin("satpasses", false)) {
    prefs.putUInt("custom", customCatalog);
    prefs.end();
  }
  editingCatalog = false;
  selectedTarget = 3;
  elementAgeDays = NAN;
  loadSelectedCache();
  calculatePass();
}

bool satellitePassEditingCatalog() {
  return editingCatalog;
}

void buildSatellitePassOledLines(
    String lines[PI_MONITOR_OLED_MESSAGE_LINE_COUNT]) {
  lines[0] = "Sat Passes";
  lines[1] = selectedLabel();
  lines[2] = editingCatalog ? "Digits: catalog ID" : "Arrows: select sat";
  lines[3] = editingCatalog ? "OK save Del erase" : "N: custom ID";
  lines[4] = editingCatalog ? "Del empty cancels" : "U: fetch TLE";
  lines[5] = "OK/R: recalc";
  lines[6] = editingCatalog ? "List Back: menu" : "Back: menu";
  lines[7] = passAvailable ? String("Peak ") + utcText(peakUnix) :
                             trackerStatus;
}
