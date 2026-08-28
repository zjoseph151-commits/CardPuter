#include "app.h"

#include <TinyGPSPlus.h>

namespace {

HardwareSerial loraGnssSerial(1);
TinyGPSPlus loraGps;
String loraGnssLineBuffer;

String clippedGnssText(const String& text, int maxChars) {
  return text.substring(0, maxChars);
}

bool nmeaField(const String& sentence, int fieldIndex, String& field) {
  int currentField = 0;
  int fieldStart = 0;

  for (int i = 0; i <= sentence.length(); ++i) {
    if (i == sentence.length() || sentence.charAt(i) == ',' ||
        sentence.charAt(i) == '*') {
      if (currentField == fieldIndex) {
        field = sentence.substring(fieldStart, i);
        field.trim();
        return true;
      }

      currentField++;
      fieldStart = i + 1;
    }
  }

  return false;
}

bool nmeaIntField(const String& sentence, int fieldIndex, int& value) {
  String field;
  if (!nmeaField(sentence, fieldIndex, field) || field.length() == 0) {
    return false;
  }

  for (int i = 0; i < field.length(); ++i) {
    const char ch = field.charAt(i);
    if (!(isDigit(ch) || (i == 0 && ch == '-'))) {
      return false;
    }
  }

  value = field.toInt();
  return true;
}

bool isGsvSentence(const String& sentence) {
  return sentence.length() >= 6 && sentence.charAt(0) == '$' &&
         sentence.charAt(3) == 'G' && sentence.charAt(4) == 'S' &&
         sentence.charAt(5) == 'V';
}

char gsvConstellation(const String& sentence) {
  if (sentence.length() < 3) {
    return '?';
  }

  const char talker = sentence.charAt(2);
  switch (talker) {
    case 'P':
      return 'G';
    case 'L':
      return 'R';
    case 'A':
      return 'E';
    case 'B':
      return 'B';
    case 'Q':
      return 'Q';
    case 'N':
      return 'N';
    default:
      return '?';
  }
}

int findGnssSkySatelliteSlot(char constellation, uint16_t prn) {
  int firstFreeSlot = -1;

  for (int i = 0; i < GNSS_SKY_MAX_SATELLITES; ++i) {
    if (gnssSkySatellites[i].active) {
      if (gnssSkySatellites[i].constellation == constellation &&
          gnssSkySatellites[i].prn == prn) {
        return i;
      }
      continue;
    }

    if (firstFreeSlot < 0) {
      firstFreeSlot = i;
    }
  }

  return firstFreeSlot;
}

int firstActiveGnssSkySatelliteIndex() {
  for (int i = 0; i < GNSS_SKY_MAX_SATELLITES; ++i) {
    if (gnssSkySatellites[i].active) {
      return i;
    }
  }

  return -1;
}

void updateGnssSkySatellite(char constellation, int prn, int elevationDeg,
                            int azimuthDeg, int snrDb) {
  if (prn <= 0) {
    return;
  }

  const int slot = findGnssSkySatelliteSlot(constellation, prn);
  if (slot < 0) {
    return;
  }

  GnssSkySatellite& satellite = gnssSkySatellites[slot];
  satellite.active = true;
  satellite.constellation = constellation;
  satellite.prn = static_cast<uint16_t>(prn);
  satellite.elevationDeg =
      static_cast<int16_t>(constrain(elevationDeg, 0, 90));
  satellite.azimuthDeg =
      static_cast<uint16_t>(constrain(azimuthDeg, 0, 359));
  satellite.snrDb = static_cast<int16_t>(snrDb);
  satellite.lastSeenMs = millis();
}

void parseGsvSentence(const String& sentence) {
  if (!isGsvSentence(sentence)) {
    return;
  }

  int satellitesInView = 0;
  if (nmeaIntField(sentence, 3, satellitesInView)) {
    gnssSkySatellitesInView = max(0, satellitesInView);
  }

  gnssSkyGsvSentenceCount++;
  lastGnssSkyGsvMs = millis();

  const char constellation = gsvConstellation(sentence);
  for (int satelliteIndex = 0; satelliteIndex < 4; ++satelliteIndex) {
    const int baseField = 4 + (satelliteIndex * 4);
    int prn = 0;
    int elevationDeg = 0;
    int azimuthDeg = 0;
    int snrDb = -1;

    if (!nmeaIntField(sentence, baseField, prn) ||
        !nmeaIntField(sentence, baseField + 1, elevationDeg) ||
        !nmeaIntField(sentence, baseField + 2, azimuthDeg)) {
      continue;
    }

    nmeaIntField(sentence, baseField + 3, snrDb);
    updateGnssSkySatellite(constellation, prn, elevationDeg, azimuthDeg,
                           snrDb);
  }

  refreshGnssSkySatellites();
}

void updateLoraGnssParsedState() {
  loraGnssCharsParsed = loraGps.charsProcessed();
  loraGnssPassedChecksum = loraGps.passedChecksum();
  loraGnssFailedChecksum = loraGps.failedChecksum();

  loraGnssLocationValid = loraGps.location.isValid();
  if (loraGnssLocationValid) {
    loraGnssLatitude = loraGps.location.lat();
    loraGnssLongitude = loraGps.location.lng();
    loraGnssFixAgeMs = loraGps.location.age();
  } else {
    loraGnssFixAgeMs = 0;
  }

  loraGnssSatellitesValid = loraGps.satellites.isValid();
  if (loraGnssSatellitesValid) {
    loraGnssSatellites = loraGps.satellites.value();
  }

  loraGnssHdopValid = loraGps.hdop.isValid();
  if (loraGnssHdopValid) {
    loraGnssHdop = loraGps.hdop.hdop();
  }

  loraGnssTimeValid = loraGps.time.isValid();
  if (loraGnssTimeValid) {
    loraGnssHour = loraGps.time.hour();
    loraGnssMinute = loraGps.time.minute();
    loraGnssSecond = loraGps.time.second();
  }

  loraGnssDateValid = loraGps.date.isValid();
  if (loraGnssDateValid) {
    loraGnssYear = loraGps.date.year();
    loraGnssMonth = loraGps.date.month();
    loraGnssDay = loraGps.date.day();
  }

  loraGnssSpeedValid = loraGps.speed.isValid();
  if (loraGnssSpeedValid) {
    loraGnssSpeedKmph = loraGps.speed.kmph();
  }

  loraGnssAltitudeValid = loraGps.altitude.isValid();
  if (loraGnssAltitudeValid) {
    loraGnssAltitudeMeters = loraGps.altitude.meters();
  }

  if (loraGnssHasFreshFix()) {
    loraGnssStatus = "GNSS fix";
  } else if (loraGnssCharsParsed > 0) {
    loraGnssStatus = "No fix yet";
  } else {
    loraGnssStatus = "Waiting NMEA";
  }
}

}  // namespace

bool loraGnssHasFreshFix() {
  return loraGnssLocationValid && loraGnssFixAgeMs <= LORA_GNSS_FIX_STALE_MS;
}

String loraGnssSatellitesText() {
  if (!loraGnssSatellitesValid) {
    return "--";
  }

  return String(loraGnssSatellites);
}

String loraGnssHdopText() {
  if (!loraGnssHdopValid) {
    return "--";
  }

  char text[10];
  snprintf(text, sizeof(text), "%.1f", loraGnssHdop);
  return text;
}

String loraGnssCoordinateText(const char* label, double value) {
  if (!loraGnssLocationValid) {
    return String(label) + ": waiting";
  }

  char text[24];
  snprintf(text, sizeof(text), "%s:%+.6f", label, value);
  return text;
}

String loraGnssUtcText() {
  if (!loraGnssTimeValid) {
    return "UTC: --:--:--";
  }

  char text[16];
  snprintf(text, sizeof(text), "UTC:%02u:%02u:%02u", loraGnssHour,
           loraGnssMinute, loraGnssSecond);
  return text;
}

String loraGnssDateText() {
  if (!loraGnssDateValid) {
    return "Date: ----/--/--";
  }

  char text[18];
  snprintf(text, sizeof(text), "Date:%04u-%02u-%02u", loraGnssYear,
           loraGnssMonth, loraGnssDay);
  return text;
}

String loraGnssTimeText() {
  if (!loraGnssTimeValid) {
    return loraLastNmeaLine.length() > 0
               ? String("NMEA:") + clippedGnssText(loraLastNmeaLine, 18)
               : loraGnssStatus;
  }

  char text[28];
  snprintf(text, sizeof(text), "UTC:%02u:%02u:%02u Age:%lus", loraGnssHour,
           loraGnssMinute, loraGnssSecond,
           static_cast<unsigned long>(loraGnssFixAgeMs / 1000UL));
  return text;
}

String loraGnssSpeedText() {
  if (!loraGnssSpeedValid) {
    return "--";
  }

  char text[14];
  snprintf(text, sizeof(text), "%.1fkmh", loraGnssSpeedKmph);
  return text;
}

String loraGnssAltitudeText() {
  if (!loraGnssAltitudeValid) {
    return "--";
  }

  char text[14];
  snprintf(text, sizeof(text), "%.0fm", loraGnssAltitudeMeters);
  return text;
}

void startLoraGnssSerial() {
  if (loraGnssStarted) {
    return;
  }

  loraGnssSerial.begin(LORA_GNSS_BAUD, SERIAL_8N1, LORA_GNSS_RX_PIN,
                       LORA_GNSS_TX_PIN);
  loraGnssStarted = true;
  loraGnssStatus = "Listening 115200";
  Serial.printf("LoRa GNSS: UART started RX=G%d TX=G%d baud=%lu.\n",
                LORA_GNSS_RX_PIN, LORA_GNSS_TX_PIN,
                static_cast<unsigned long>(LORA_GNSS_BAUD));
}

void serviceLoraGnssSerial() {
  if (!loraGnssStarted) {
    return;
  }

  bool parsedBytes = false;

  while (loraGnssSerial.available() > 0) {
    const char ch = static_cast<char>(loraGnssSerial.read());
    loraGnssByteCount++;
    loraGps.encode(ch);
    parsedBytes = true;

    if (ch == '\r') {
      continue;
    }

    if (ch == '\n') {
      loraGnssLineBuffer.trim();
      if (loraGnssLineBuffer.length() > 0) {
        parseGsvSentence(loraGnssLineBuffer);
        loraLastNmeaLine = loraGnssLineBuffer;
        loraGnssLineCount++;
        loraGnssStatus = "NMEA received";
      }
      loraGnssLineBuffer = "";
      continue;
    }

    if (loraGnssLineBuffer.length() < LORA_GNSS_MAX_LINE_CHARS) {
      loraGnssLineBuffer += ch;
    }
  }

  if (loraGnssByteCount == 0) {
    loraGnssStatus = "Waiting NMEA";
  }

  if (parsedBytes) {
    updateLoraGnssParsedState();
    refreshGnssSkySatellites();
  }
}

void resetLoraGnssParser() {
  loraGnssStatus = "Not started.";
  loraLastNmeaLine = "";
  loraGnssLineBuffer = "";
  loraGps = TinyGPSPlus();
  loraGnssByteCount = 0;
  loraGnssLineCount = 0;
  loraGnssLocationValid = false;
  loraGnssSatellitesValid = false;
  loraGnssHdopValid = false;
  loraGnssTimeValid = false;
  loraGnssDateValid = false;
  loraGnssSpeedValid = false;
  loraGnssAltitudeValid = false;
  loraGnssLatitude = 0.0;
  loraGnssLongitude = 0.0;
  loraGnssHdop = 0.0f;
  loraGnssSpeedKmph = 0.0f;
  loraGnssAltitudeMeters = 0.0f;
  loraGnssCharsParsed = 0;
  loraGnssPassedChecksum = 0;
  loraGnssFailedChecksum = 0;
  loraGnssFixAgeMs = 0;
  loraGnssSatellites = 0;
  loraGnssYear = 0;
  loraGnssMonth = 0;
  loraGnssDay = 0;
  loraGnssHour = 0;
  loraGnssMinute = 0;
  loraGnssSecond = 0;
  gnssSkySatelliteCount = 0;
  gnssSkySatellitesInView = 0;
  gnssSkyGsvSentenceCount = 0;
  selectedGnssSkySatelliteIndex = -1;
  lastGnssSkyGsvMs = 0;

  for (int i = 0; i < GNSS_SKY_MAX_SATELLITES; ++i) {
    gnssSkySatellites[i] = GnssSkySatellite();
  }
}

void stopLoraGnssSerial() {
  if (loraGnssStarted) {
    loraGnssSerial.end();
  }

  loraGnssStarted = false;
}

void refreshGnssSkySatellites() {
  const unsigned long now = millis();
  uint32_t activeCount = 0;
  bool selectedStillActive = false;

  for (int i = 0; i < GNSS_SKY_MAX_SATELLITES; ++i) {
    GnssSkySatellite& satellite = gnssSkySatellites[i];
    if (!satellite.active) {
      continue;
    }

    if (now - satellite.lastSeenMs > GNSS_SKY_STALE_MS) {
      satellite = GnssSkySatellite();
      continue;
    }

    if (i == selectedGnssSkySatelliteIndex) {
      selectedStillActive = true;
    }
    activeCount++;
  }

  gnssSkySatelliteCount = activeCount;

  if (activeCount == 0) {
    selectedGnssSkySatelliteIndex = -1;
  } else if (!selectedStillActive) {
    selectedGnssSkySatelliteIndex = firstActiveGnssSkySatelliteIndex();
  }
}
