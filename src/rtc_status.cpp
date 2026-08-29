#include "app.h"

namespace {

uint8_t bcdToDecimal(uint8_t value) {
  return ((value >> 4) * 10U) + (value & 0x0F);
}

uint8_t decimalToBcd(uint8_t value) {
  return ((value / 10U) << 4) | (value % 10U);
}

String clippedRtcText(const String& text, int maxChars) {
  return text.substring(0, maxChars);
}

uint8_t buildMonthFromText(const char* monthText) {
  const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  for (uint8_t i = 0; i < 12; ++i) {
    if (strncmp(monthText, months[i], 3) == 0) {
      return i + 1;
    }
  }

  return 0;
}

uint8_t dayOfWeekFromDate(uint16_t year, uint8_t month, uint8_t day) {
  static const uint8_t offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  int adjustedYear = year;

  if (month < 3) {
    --adjustedYear;
  }

  const int zeroSunday =
      (adjustedYear + adjustedYear / 4 - adjustedYear / 100 +
       adjustedYear / 400 + offsets[month - 1] + day) %
      7;
  return static_cast<uint8_t>(zeroSunday + 1);
}

bool isRtcLeapYear(uint16_t year) {
  return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
}

uint8_t rtcDaysInMonth(uint16_t year, uint8_t month) {
  static const uint8_t days[] = {31, 28, 31, 30, 31, 30,
                                 31, 31, 30, 31, 30, 31};

  if (month == 2 && isRtcLeapYear(year)) {
    return 29;
  }

  if (month < 1 || month > 12) {
    return 31;
  }

  return days[month - 1];
}

void advanceRtcDateTimeBySeconds(uint16_t& year, uint8_t& month, uint8_t& day,
                                 uint8_t& hour, uint8_t& minute,
                                 uint8_t& second, uint32_t secondsToAdd) {
  uint32_t totalSeconds = static_cast<uint32_t>(second) + secondsToAdd;
  second = static_cast<uint8_t>(totalSeconds % 60U);

  uint32_t totalMinutes =
      static_cast<uint32_t>(minute) + (totalSeconds / 60U);
  minute = static_cast<uint8_t>(totalMinutes % 60U);

  uint32_t totalHours = static_cast<uint32_t>(hour) + (totalMinutes / 60U);
  hour = static_cast<uint8_t>(totalHours % 24U);

  uint32_t daysToAdd = totalHours / 24U;
  while (daysToAdd > 0) {
    const uint8_t daysThisMonth = rtcDaysInMonth(year, month);
    const uint8_t remainingDaysThisMonth = daysThisMonth - day;

    if (daysToAdd <= remainingDaysThisMonth) {
      day = static_cast<uint8_t>(day + daysToAdd);
      break;
    }

    daysToAdd -= static_cast<uint32_t>(remainingDaysThisMonth) + 1U;
    day = 1;
    ++month;

    if (month > 12) {
      month = 1;
      ++year;
    }
  }
}

bool parseBuildDateTime(uint16_t& year, uint8_t& month, uint8_t& day,
                        uint8_t& dayOfWeek, uint8_t& hour, uint8_t& minute,
                        uint8_t& second) {
  char monthText[4] = {};
  int parsedYear = 0;
  int parsedDay = 0;
  int parsedHour = 0;
  int parsedMinute = 0;
  int parsedSecond = 0;

  if (sscanf(__DATE__, "%3s %d %d", monthText, &parsedDay, &parsedYear) != 3 ||
      sscanf(__TIME__, "%d:%d:%d", &parsedHour, &parsedMinute,
             &parsedSecond) != 3) {
    return false;
  }

  month = buildMonthFromText(monthText);
  if (month == 0 || parsedYear < 2000 || parsedYear > 2099 ||
      parsedDay < 1 || parsedDay > 31 || parsedHour < 0 || parsedHour > 23 ||
      parsedMinute < 0 || parsedMinute > 59 || parsedSecond < 0 ||
      parsedSecond > 59) {
    return false;
  }

  year = static_cast<uint16_t>(parsedYear);
  day = static_cast<uint8_t>(parsedDay);
  hour = static_cast<uint8_t>(parsedHour);
  minute = static_cast<uint8_t>(parsedMinute);
  second = static_cast<uint8_t>(parsedSecond);
  dayOfWeek = dayOfWeekFromDate(year, month, day);
  return true;
}

bool readNtpLocalTime(tm& timeInfo) {
  if (WiFi.status() != WL_CONNECTED) {
    rtcStatus = "Use WiFi Connect";
    Serial.println("RTC: NTP set skipped; Wi-Fi is not connected.");
    return false;
  }

  configTzTime(RTC_TIMEZONE_POSIX, RTC_NTP_SERVER_PRIMARY,
               RTC_NTP_SERVER_SECONDARY);

  const unsigned long startedAtMs = millis();
  while (millis() - startedAtMs < RTC_NTP_SYNC_TIMEOUT_MS) {
    if (getLocalTime(&timeInfo, RTC_NTP_POLL_MS)) {
      const int year = timeInfo.tm_year + 1900;
      if (year >= 2000 && year <= 2099) {
        return true;
      }
    }

    M5Cardputer.update();
    delay(25);
  }

  rtcStatus = "NTP sync failed";
  Serial.println("RTC: NTP sync timed out.");
  return false;
}

bool probeRtcAddress(uint8_t address) {
  if (!selectRtcI2cPath()) {
    return false;
  }

  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

bool readDs3231Registers(uint8_t startRegister, uint8_t* buffer,
                         uint8_t length) {
  if (!selectRtcI2cPath()) {
    return false;
  }

  Wire.beginTransmission(RTC_DS3231_ADDRESS);
  Wire.write(startRegister);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  const uint8_t bytesRead = Wire.requestFrom(RTC_DS3231_ADDRESS, length);
  if (bytesRead != length) {
    while (Wire.available()) {
      Wire.read();
    }
    return false;
  }

  for (uint8_t i = 0; i < length; ++i) {
    if (!Wire.available()) {
      return false;
    }
    buffer[i] = Wire.read();
  }

  return true;
}

bool readDs3231Register(uint8_t reg, uint8_t& value) {
  return readDs3231Registers(reg, &value, 1);
}

bool writeDs3231Register(uint8_t reg, uint8_t value) {
  if (!selectRtcI2cPath()) {
    return false;
  }

  Wire.beginTransmission(RTC_DS3231_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool writeDs3231TimeRegisters(uint16_t year, uint8_t month, uint8_t day,
                              uint8_t dayOfWeek, uint8_t hour, uint8_t minute,
                              uint8_t second) {
  if (!selectRtcI2cPath()) {
    return false;
  }

  Wire.beginTransmission(RTC_DS3231_ADDRESS);
  Wire.write(RTC_DS3231_SECONDS_REGISTER);
  Wire.write(decimalToBcd(second));
  Wire.write(decimalToBcd(minute));
  Wire.write(decimalToBcd(hour));
  Wire.write(decimalToBcd(dayOfWeek));
  Wire.write(decimalToBcd(day));
  Wire.write(decimalToBcd(month));
  Wire.write(decimalToBcd(static_cast<uint8_t>(year - 2000U)));
  return Wire.endTransmission() == 0;
}

bool decodeDs3231Time(const uint8_t registers[7]) {
  rtcSecond = bcdToDecimal(registers[0] & 0x7F);
  rtcMinute = bcdToDecimal(registers[1] & 0x7F);

  const uint8_t hourRegister = registers[2];
  if ((hourRegister & 0x40) != 0) {
    rtcHour = bcdToDecimal(hourRegister & 0x1F);
    const bool isPm = (hourRegister & 0x20) != 0;
    if (rtcHour == 12) {
      rtcHour = isPm ? 12 : 0;
    } else if (isPm) {
      rtcHour += 12;
    }
  } else {
    rtcHour = bcdToDecimal(hourRegister & 0x3F);
  }

  rtcDayOfWeek = bcdToDecimal(registers[3] & 0x07);
  rtcDay = bcdToDecimal(registers[4] & 0x3F);
  rtcMonth = bcdToDecimal(registers[5] & 0x1F);
  rtcYear = 2000U + bcdToDecimal(registers[6]);

  return rtcSecond < 60 && rtcMinute < 60 && rtcHour < 24 &&
         rtcDayOfWeek >= 1 && rtcDayOfWeek <= 7 && rtcDay >= 1 &&
         rtcDay <= 31 && rtcMonth >= 1 && rtcMonth <= 12 &&
         rtcYear >= 2000 && rtcYear <= 2099;
}

void readDs3231Temperature() {
  uint8_t temperatureRegisters[2] = {};
  rtcTemperatureValid = false;

  if (!readDs3231Registers(RTC_DS3231_TEMPERATURE_REGISTER,
                           temperatureRegisters,
                           sizeof(temperatureRegisters))) {
    return;
  }

  const int8_t wholeDegrees = static_cast<int8_t>(temperatureRegisters[0]);
  const float fractionalDegrees =
      static_cast<float>((temperatureRegisters[1] >> 6) & 0x03) * 0.25f;
  rtcTemperatureC = static_cast<float>(wholeDegrees) + fractionalDegrees;
  rtcTemperatureValid = true;
}

}  // namespace

void showRtcStatus() {
  if (!rtcInitialized) {
    initRtcStatus();
  } else {
    readRtcStatus();
  }

  renderRtcStatus();
}

void renderRtcStatus() {
  beginContentDraw();
  contentCanvas.printf("Status: %s\n",
                       clippedRtcText(rtcStatus, 19).c_str());
  contentCanvas.printf("DS:%s EE:%s Rd:%lu\n",
                       rtcOnline ? "online" : "missing",
                       rtcEepromDetected ? "seen" : "--",
                       static_cast<unsigned long>(rtcReadAttemptCount));
  contentCanvas.println(rtcDateText());
  contentCanvas.println(rtcTimeText());
  contentCanvas.println(rtcTemperatureText());
  contentCanvas.println("N NTP S build OK/R");
  commitContentDraw();
}

bool initRtcStatus() {
  resetRtcStatus();
  rtcInitialized = true;
  rtcStatus = "Starting...";

  Wire.begin(ENV_I2C_SDA_PIN, ENV_I2C_SCL_PIN, ENV_I2C_FREQUENCY);
  detectI2cHub();

  return readRtcStatus();
}

void serviceRtcStatus() {
  const unsigned long now = millis();

  if (!rtcInitialized) {
    initRtcStatus();
    renderRtcStatus();
    return;
  }

  const uint32_t interval =
      rtcOnline ? RTC_REFRESH_INTERVAL_MS : RTC_RETRY_INTERVAL_MS;
  const unsigned long lastAttempt =
      rtcOnline ? lastRtcRefreshMs : lastRtcRetryMs;

  if (now - lastAttempt < interval) {
    return;
  }

  readRtcStatus();

  if (currentScreen == Screen::RtcStatus) {
    renderRtcStatus();
  }
}

void resetRtcStatus() {
  rtcInitialized = false;
  rtcOnline = false;
  rtcEepromDetected = false;
  rtcTimeValid = false;
  rtcOscillatorStopped = false;
  rtcTemperatureValid = false;
  rtcYear = 0;
  rtcMonth = 0;
  rtcDay = 0;
  rtcHour = 0;
  rtcMinute = 0;
  rtcSecond = 0;
  rtcDayOfWeek = 0;
  rtcTemperatureC = 0.0f;
  lastRtcRefreshMs = 0;
  lastRtcRetryMs = 0;
  rtcStatus = "Not initialized.";
}

bool readRtcStatus() {
  rtcReadAttemptCount++;

  if (!i2cHubDetected) {
    detectI2cHub();
  }

  if (!selectRtcI2cPath()) {
    rtcOnline = false;
    rtcTimeValid = false;
    rtcTemperatureValid = false;
    rtcStatus = "PaHub ch5 missing";
    lastRtcRetryMs = millis();
    return false;
  }

  rtcEepromDetected = probeRtcAddress(RTC_AT24C32_ADDRESS);

  if (!probeRtcAddress(RTC_DS3231_ADDRESS)) {
    rtcOnline = false;
    rtcTimeValid = false;
    rtcTemperatureValid = false;
    rtcStatus = "DS3231 not found";
    lastRtcRetryMs = millis();
    return false;
  }

  uint8_t timeRegisters[7] = {};
  if (!readDs3231Registers(RTC_DS3231_SECONDS_REGISTER, timeRegisters,
                           sizeof(timeRegisters))) {
    rtcOnline = false;
    rtcTimeValid = false;
    rtcTemperatureValid = false;
    rtcStatus = "RTC read failed";
    lastRtcRetryMs = millis();
    return false;
  }

  rtcOnline = true;
  rtcTimeValid = decodeDs3231Time(timeRegisters);

  uint8_t statusRegister = 0;
  if (readDs3231Register(RTC_DS3231_STATUS_REGISTER, statusRegister)) {
    rtcOscillatorStopped = (statusRegister & 0x80) != 0;
  } else {
    rtcOscillatorStopped = false;
  }

  readDs3231Temperature();

  if (!rtcTimeValid) {
    rtcStatus = "RTC time invalid";
  } else if (rtcOscillatorStopped) {
    rtcStatus = "RTC needs set";
  } else {
    rtcStatus = "RTC online";
  }

  lastRtcRefreshMs = millis();
  return true;
}

bool setRtcToBuildTime() {
  uint16_t buildYear = 0;
  uint8_t buildMonth = 0;
  uint8_t buildDay = 0;
  uint8_t buildDayOfWeek = 0;
  uint8_t buildHour = 0;
  uint8_t buildMinute = 0;
  uint8_t buildSecond = 0;

  if (!parseBuildDateTime(buildYear, buildMonth, buildDay, buildDayOfWeek,
                          buildHour, buildMinute, buildSecond)) {
    rtcStatus = "Build time parse fail";
    Serial.println("RTC: could not parse firmware build date/time.");
    return false;
  }

  advanceRtcDateTimeBySeconds(buildYear, buildMonth, buildDay, buildHour,
                              buildMinute, buildSecond,
                              millis() / 1000UL);
  buildDayOfWeek = dayOfWeekFromDate(buildYear, buildMonth, buildDay);

  if (!i2cHubDetected) {
    detectI2cHub();
  }

  if (!selectRtcI2cPath()) {
    rtcOnline = false;
    rtcStatus = "PaHub ch5 missing";
    Serial.println("RTC: set skipped; PaHub channel 5 is unavailable.");
    return false;
  }

  if (!probeRtcAddress(RTC_DS3231_ADDRESS)) {
    rtcOnline = false;
    rtcStatus = "DS3231 not found";
    Serial.println("RTC: set skipped; DS3231 not found on PaHub channel 5.");
    return false;
  }

  uint8_t controlRegister = 0;
  if (readDs3231Register(RTC_DS3231_CONTROL_REGISTER, controlRegister)) {
    writeDs3231Register(RTC_DS3231_CONTROL_REGISTER,
                        controlRegister & static_cast<uint8_t>(~0x80));
  }

  if (!writeDs3231TimeRegisters(buildYear, buildMonth, buildDay,
                                buildDayOfWeek, buildHour, buildMinute,
                                buildSecond)) {
    rtcStatus = "RTC set failed";
    Serial.println("RTC: DS3231 time write failed.");
    return false;
  }

  uint8_t statusRegister = 0;
  if (readDs3231Register(RTC_DS3231_STATUS_REGISTER, statusRegister)) {
    writeDs3231Register(RTC_DS3231_STATUS_REGISTER,
                        statusRegister & static_cast<uint8_t>(~0x80));
  }

  readRtcStatus();
  rtcStatus = "RTC set build";
  Serial.printf("RTC: set DS3231 from firmware build time %04u-%02u-%02u "
                "%02u:%02u:%02u.\n",
                static_cast<unsigned int>(buildYear), buildMonth, buildDay,
                buildHour, buildMinute, buildSecond);
  return true;
}

bool setRtcFromNtp() {
  tm timeInfo = {};
  rtcStatus = "NTP sync...";

  if (!readNtpLocalTime(timeInfo)) {
    return false;
  }

  if (!i2cHubDetected) {
    detectI2cHub();
  }

  if (!selectRtcI2cPath()) {
    rtcOnline = false;
    rtcStatus = "PaHub ch5 missing";
    Serial.println("RTC: NTP set skipped; PaHub channel 5 is unavailable.");
    return false;
  }

  if (!probeRtcAddress(RTC_DS3231_ADDRESS)) {
    rtcOnline = false;
    rtcStatus = "DS3231 not found";
    Serial.println("RTC: NTP set skipped; DS3231 not found on PaHub channel 5.");
    return false;
  }

  uint8_t controlRegister = 0;
  if (readDs3231Register(RTC_DS3231_CONTROL_REGISTER, controlRegister)) {
    writeDs3231Register(RTC_DS3231_CONTROL_REGISTER,
                        controlRegister & static_cast<uint8_t>(~0x80));
  }

  const uint16_t ntpYear = static_cast<uint16_t>(timeInfo.tm_year + 1900);
  const uint8_t ntpMonth = static_cast<uint8_t>(timeInfo.tm_mon + 1);
  const uint8_t ntpDay = static_cast<uint8_t>(timeInfo.tm_mday);
  const uint8_t ntpDayOfWeek = static_cast<uint8_t>(timeInfo.tm_wday + 1);
  const uint8_t ntpHour = static_cast<uint8_t>(timeInfo.tm_hour);
  const uint8_t ntpMinute = static_cast<uint8_t>(timeInfo.tm_min);
  const uint8_t ntpSecond = static_cast<uint8_t>(timeInfo.tm_sec);

  if (!writeDs3231TimeRegisters(ntpYear, ntpMonth, ntpDay, ntpDayOfWeek,
                                ntpHour, ntpMinute, ntpSecond)) {
    rtcStatus = "RTC NTP set failed";
    Serial.println("RTC: DS3231 NTP time write failed.");
    return false;
  }

  uint8_t statusRegister = 0;
  if (readDs3231Register(RTC_DS3231_STATUS_REGISTER, statusRegister)) {
    writeDs3231Register(RTC_DS3231_STATUS_REGISTER,
                        statusRegister & static_cast<uint8_t>(~0x80));
  }

  readRtcStatus();
  rtcStatus = "RTC set NTP";
  Serial.printf("RTC: set DS3231 from NTP local time %04u-%02u-%02u "
                "%02u:%02u:%02u.\n",
                static_cast<unsigned int>(ntpYear), ntpMonth, ntpDay,
                ntpHour, ntpMinute, ntpSecond);
  return true;
}

String rtcDateText() {
  if (!rtcOnline) {
    return "Date: --";
  }

  if (!rtcTimeValid) {
    return "Date: invalid";
  }

  char text[18];
  snprintf(text, sizeof(text), "Date: %04u-%02u-%02u",
           static_cast<unsigned int>(rtcYear), rtcMonth, rtcDay);
  return text;
}

String rtcTimeText() {
  if (!rtcOnline) {
    return "Time: --";
  }

  if (!rtcTimeValid) {
    return "Time: invalid";
  }

  char text[15];
  snprintf(text, sizeof(text), "Time: %02u:%02u:%02u", rtcHour, rtcMinute,
           rtcSecond);
  return text;
}

String rtcTemperatureText() {
  if (!rtcOnline || !rtcTemperatureValid) {
    return "Temp: --";
  }

  char text[18];
  snprintf(text, sizeof(text), "Temp: %.2f C", rtcTemperatureC);
  return text;
}
