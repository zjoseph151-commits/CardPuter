#include "app.h"

namespace {

uint32_t lastPiMonitorOledMessageSequence = 0;
uint8_t piMonitorOledPageIndex = 0;
String lastPiMonitorOledScope;

constexpr uint8_t OLED_HELP_LINE_COUNT = PI_MONITOR_OLED_MESSAGE_LINE_COUNT;
constexpr uint8_t OLED_HELP_MAX_CHARS = PI_MONITOR_OLED_MESSAGE_MAX_CHARS;

String screenTitleForOled() {
  switch (currentScreen) {
    case Screen::MainMenu:
      return "Scoober";
    case Screen::BatteryInfo:
      return "Battery";
    case Screen::SystemInfo:
      return "System";
    case Screen::WifiScan:
      return "WiFi Scan";
    case Screen::WifiSaveConfirm:
    case Screen::WifiSaveResult:
      return "Save WiFi";
    case Screen::SavedWifi:
    case Screen::SavedWifiDeleteConfirm:
    case Screen::SavedWifiDeleteResult:
      return "Saved WiFi";
    case Screen::WifiConnect:
      return "WiFi Connect";
    case Screen::PiMonitor:
      return "Pi Monitor";
    case Screen::SdManager:
      return "SD Manager";
    case Screen::VoiceMemos:
    case Screen::VoiceMemoDeleteConfirm:
    case Screen::VoiceMemoDeleteResult:
      return "Voice Memos";
    case Screen::Environment:
    case Screen::EnvironmentLogName:
      return "Environment";
    case Screen::OledTest:
      return "OLED Test";
    case Screen::RtcStatus:
      return "RTC";
    case Screen::GnssDashboard:
      return "GNSS Dash";
    case Screen::GnssSkyView:
      return "GNSS Sky";
    case Screen::ReturnHome:
      return "Return Home";
    case Screen::BreadcrumbLogger:
      return "Breadcrumbs";
    case Screen::LoraMessages:
      return "LoRa Messages";
    case Screen::LoraDiag:
      return "LoRa Diag";
    case Screen::LevelTool:
      return "Level";
    default:
      return "Scoober";
  }
}

String clippedOledText(const String& text) {
  return text.substring(0, OLED_STATUS_MAX_CHARS);
}

String formatI2cAddress(uint8_t address) {
  char buffer[6];
  snprintf(buffer, sizeof(buffer), "0x%02X", address);
  return String(buffer);
}

uint32_t currentOledBusFrequency() {
  return oledActiveBusFrequency > 0 ? oledActiveBusFrequency
                                    : OLED_I2C_FAST_FREQUENCY;
}

void setOledBusClock() {
  const uint32_t frequency = currentOledBusFrequency();
  Wire.setClock(frequency);
  oledDisplay.setBusClock(frequency);
}

void restoreExternalI2cBusClock() {
  Wire.setClock(ENV_I2C_FREQUENCY);
}

void configureExternalI2cForOled(uint32_t frequency) {
  static bool externalI2cStarted = false;
  if (!externalI2cStarted && !i2cHubDetected) {
    Wire.begin(ENV_I2C_SDA_PIN, ENV_I2C_SCL_PIN, frequency);
    externalI2cStarted = true;
  }

  Wire.setClock(frequency);
  delay(2);
}

bool probeSelectedI2cAddress(uint8_t address, uint8_t* error = nullptr) {
  Wire.beginTransmission(address);
  const uint8_t result = Wire.endTransmission();
  if (error != nullptr) {
    *error = result;
  }

  return result == 0;
}

String oledProbeFailureSummary(uint32_t frequency, uint8_t primaryError,
                               uint8_t secondaryError) {
  return String("ch") + String(I2C_HUB_OLED_CHANNEL) + " " +
         formatI2cAddress(OLED_I2C_ADDRESS_PRIMARY) + "/" +
         formatI2cAddress(OLED_I2C_ADDRESS_SECONDARY) + " e" +
         String(primaryError) + "/" + String(secondaryError) + " @" +
         String(frequency / 1000UL) + "k";
}

bool tryOledProbeAtFrequency(uint32_t frequency) {
  oledActiveBusFrequency = frequency;
  configureExternalI2cForOled(frequency);

  if (!detectI2cHub()) {
    oledScanSummary = "No PaHub";
    return false;
  }

  if (!selectOledI2cPath()) {
    oledScanSummary = "PaHub select fail";
    return false;
  }

  uint8_t lastPrimaryError = 0;
  uint8_t lastSecondaryError = 0;
  for (uint8_t attempt = 0; attempt < OLED_PROBE_ATTEMPTS; ++attempt) {
    if (probeSelectedI2cAddress(OLED_I2C_ADDRESS_PRIMARY, &lastPrimaryError)) {
      oledActiveAddress = OLED_I2C_ADDRESS_PRIMARY;
      oledActiveBusFrequency = frequency;
      oledScanSummary = String("ch") + String(I2C_HUB_OLED_CHANNEL) + " " +
                        formatI2cAddress(oledActiveAddress) + " @" +
                        String(frequency / 1000UL) + "k";
      return true;
    }

    if (probeSelectedI2cAddress(OLED_I2C_ADDRESS_SECONDARY,
                                &lastSecondaryError)) {
      oledActiveAddress = OLED_I2C_ADDRESS_SECONDARY;
      oledActiveBusFrequency = frequency;
      oledScanSummary = String("ch") + String(I2C_HUB_OLED_CHANNEL) + " " +
                        formatI2cAddress(oledActiveAddress) + " @" +
                        String(frequency / 1000UL) + "k";
      return true;
    }

    delay(20);
  }

  oledScanSummary =
      oledProbeFailureSummary(frequency, lastPrimaryError, lastSecondaryError);
  return false;
}

void drawOledLine(uint8_t row, const String& text) {
  if (row >= OLED_STATUS_LINE_COUNT) {
    return;
  }

  const uint8_t y = 10 + (row * 12);
  const String clipped = clippedOledText(text);
  oledDisplay.drawStr(0, y, clipped.c_str());
}

String piMonitorOledScopeLabel() {
  String scope = activePiMonitorProjectLabel();
  scope.trim();
  if (scope.length() == 0 || scope == "No project") {
    scope = "Home";
  } else if (scope == "Home / Diagnostics") {
    scope = "Home";
  }

  return scope.substring(0, 12);
}

void drawPiMonitorOledMessageLine(uint8_t row, const String& text) {
  if (row >= PI_MONITOR_OLED_MESSAGE_LINE_COUNT) {
    return;
  }

  const uint8_t y = 7 + (row * 8);
  oledDisplay.drawStr(
      0, y, text.substring(0, PI_MONITOR_OLED_MESSAGE_MAX_CHARS).c_str());
}

void drawOledHelpLine(uint8_t row, const String& text) {
  if (row >= OLED_HELP_LINE_COUNT) {
    return;
  }

  const uint8_t y = 7 + (row * 8);
  oledDisplay.drawStr(0, y, text.substring(0, OLED_HELP_MAX_CHARS).c_str());
}

void renderOledHelpLines(const String lines[OLED_HELP_LINE_COUNT]) {
  oledDrawCount++;
  oledDisplay.clearBuffer();
  setOledBusClock();
  oledDisplay.setFont(u8g2_font_5x7_tf);

  for (uint8_t row = 0; row < OLED_HELP_LINE_COUNT; ++row) {
    drawOledHelpLine(row, lines[row]);
  }

  oledDisplay.sendBuffer();
  restoreExternalI2cBusClock();
}

void appendPiMonitorOledWrappedSegment(String lines[], uint8_t& lineCount,
                                       const String& segment) {
  if (lineCount >= PI_MONITOR_OLED_WRAPPED_LINE_LIMIT) {
    return;
  }

  if (segment.length() == 0) {
    lines[lineCount++] = "";
    return;
  }

  int start = 0;
  while (start < segment.length() &&
         lineCount < PI_MONITOR_OLED_WRAPPED_LINE_LIMIT) {
    lines[lineCount++] =
        segment.substring(start, start + PI_MONITOR_OLED_MESSAGE_MAX_CHARS);
    start += PI_MONITOR_OLED_MESSAGE_MAX_CHARS;
  }
}

uint8_t buildPiMonitorOledWrappedLines(String lines[]) {
  String messageText = piMonitorLatestOledMessageText();
  messageText.replace('\r', '\n');

  uint8_t lineCount = 0;
  int start = 0;
  while (start <= messageText.length() &&
         lineCount < PI_MONITOR_OLED_WRAPPED_LINE_LIMIT) {
    int end = messageText.indexOf('\n', start);
    if (end < 0) {
      end = messageText.length();
    }

    appendPiMonitorOledWrappedSegment(lines, lineCount,
                                      messageText.substring(start, end));
    if (end >= messageText.length()) {
      break;
    }
    start = end + 1;
  }

  return lineCount;
}

void renderPiMonitorOledMessages() {
  const String scope = piMonitorOledScopeLabel();
  const uint32_t messageSequence = piMonitorLatestOledMessageSequence();
  if (messageSequence != lastPiMonitorOledMessageSequence ||
      scope != lastPiMonitorOledScope) {
    lastPiMonitorOledMessageSequence = messageSequence;
    lastPiMonitorOledScope = scope;
    piMonitorOledPageIndex = 0;
  }

  String wrappedLines[PI_MONITOR_OLED_WRAPPED_LINE_LIMIT];
  const uint8_t wrappedLineCount = buildPiMonitorOledWrappedLines(wrappedLines);
  const uint8_t pageCount =
      max(static_cast<uint8_t>(1),
          static_cast<uint8_t>((wrappedLineCount + PI_MONITOR_OLED_MESSAGE_BODY_LINES -
                                1) /
                               PI_MONITOR_OLED_MESSAGE_BODY_LINES));

  if (piMonitorOledPageIndex >= pageCount) {
    piMonitorOledPageIndex = 0;
  }

  oledDrawCount++;
  oledDisplay.clearBuffer();
  setOledBusClock();
  oledDisplay.setFont(u8g2_font_5x7_tf);

  String header =
      String("MQTT ") + scope + " S " + String(piMonitorOledPageIndex + 1) +
      "/" + String(pageCount);
  drawPiMonitorOledMessageLine(0, header);

  const uint8_t firstBodyLine =
      piMonitorOledPageIndex * PI_MONITOR_OLED_MESSAGE_BODY_LINES;
  for (uint8_t row = 0; row < PI_MONITOR_OLED_MESSAGE_BODY_LINES; ++row) {
    const uint8_t wrappedIndex = firstBodyLine + row;
    drawPiMonitorOledMessageLine(
        row + 1, wrappedIndex < wrappedLineCount ? wrappedLines[wrappedIndex]
                                                 : String(""));
  }

  oledDisplay.sendBuffer();
  restoreExternalI2cBusClock();
}

String oledBatteryLine() {
  if (isFilteredCharging()) {
    if (isBatteryLevelDisplayable()) {
      return String("Battery: ") + String(lastBatteryLevel) + "% Chg";
    }

    return "Battery: Charging";
  }

  if (isBatteryLevelDisplayable()) {
    return String("Battery: ") + String(lastBatteryLevel) + "%";
  }

  if (lastBatteryVoltageMv > 0) {
    return String("Battery: ") + String(lastBatteryVoltageMv) + "mV";
  }

  return "Battery: --";
}

String oledWifiLine() {
  return WiFi.status() == WL_CONNECTED ? "WiFi: Connected" : "WiFi: Disconnected";
}

bool piMonitorHasOledState() {
  return currentScreen == Screen::PiMonitor || piMonitorConfigLoaded ||
         piMonitorMessageCount > 0 || piMonitorStatus != "Not connected.";
}

String oledMqttLine() {
  if (!piMonitorHasOledState()) {
    return "MQTT: unused";
  }

  return piMonitorMqttClient.connected() ? "MQTT: Connected" : "MQTT: Disconnected";
}

String oledTemperatureLine() {
  if (!envHasTempHumidity) {
    return "Temp: waiting";
  }

  char line[OLED_STATUS_MAX_CHARS + 1];
  const float tempF = (envTemperatureC * 9.0f / 5.0f) + 32.0f;
  snprintf(line, sizeof(line), "Temp: %.1f F", tempF);
  return line;
}

String oledHumidityLine() {
  if (!envHasTempHumidity) {
    return "Hum: waiting";
  }

  char line[OLED_STATUS_MAX_CHARS + 1];
  snprintf(line, sizeof(line), "Hum: %.0f%%", envHumidityPercent);
  return line;
}

String oledPressureLine() {
  if (envHasPressure) {
    char line[OLED_STATUS_MAX_CHARS + 1];
    snprintf(line, sizeof(line), "Press: %.1f hPa", envPressureHpa);
    return line;
  }

  return envPressureInvalid ? "Press: invalid" : "Press: waiting";
}

String selectedVoiceMemoNameForOled() {
  if (voiceMemoRecording) {
    return activeVoiceMemoName.length() > 0 ? activeVoiceMemoName : "recording";
  }

  if (voiceMemoCount > 0 && selectedVoiceMemoIndex >= 0 &&
      selectedVoiceMemoIndex < voiceMemoCount) {
    return voiceMemos[selectedVoiceMemoIndex].name;
  }

  return voiceMemoStatus.length() > 0 ? voiceMemoStatus : "No memos";
}

String oledVoiceMemoLine() {
  if (voiceMemoRecording) {
    const uint32_t elapsedSeconds =
        (millis() - voiceMemoRecordingStartedMs) / 1000UL;
    char line[OLED_STATUS_MAX_CHARS + 1];
    snprintf(line, sizeof(line), "REC %02lu:%02lu",
             static_cast<unsigned long>(elapsedSeconds / 60UL),
             static_cast<unsigned long>(elapsedSeconds % 60UL));
    return line;
  }

  if (voiceMemoPlaying) {
    return "Playing";
  }

  return voiceSdAvailable ? String("Memos: ") + String(voiceMemoCount)
                          : "SD: missing";
}

String oledLevelLine() {
  if (!levelSmoothingInitialized) {
    return "Level: waiting";
  }

  char line[OLED_STATUS_MAX_CHARS + 1];
  snprintf(line, sizeof(line), "X:%+.2f Y:%+.2f", smoothedLevelX, smoothedLevelY);
  return line;
}

String customOrDefaultOledLine(const String& fallback) {
  String custom = oledStatusLine;
  custom.trim();
  return custom.length() > 0 ? custom : fallback;
}

void buildDefaultOledLines(String lines[OLED_STATUS_LINE_COUNT]) {
  const String title = screenTitleForOled();
  lines[0] = title;
  lines[1] = oledBatteryLine();
  lines[2] = oledWifiLine();
  lines[3] = piMonitorHasOledState() ? oledMqttLine() : String("Mode: ") + title;
  lines[4] = customOrDefaultOledLine("Status: ready");
}

void buildMainMenuOledLines(String lines[OLED_STATUS_LINE_COUNT]) {
  lines[0] = "Scoober";
  lines[1] = oledBatteryLine();
  lines[2] = oledWifiLine();
  lines[3] = piMonitorHasOledState() ? oledMqttLine() : "Mode: Menu";

  if (MENU_ITEM_COUNT > 0 && selectedMenuIndex >= 0 &&
      selectedMenuIndex < MENU_ITEM_COUNT) {
    lines[4] = String("Sel: ") + MENU_ITEMS[selectedMenuIndex].label;
  } else {
    lines[4] = "Sel: --";
  }
}

void buildPiMonitorOledLines(String lines[OLED_STATUS_LINE_COUNT]) {
  lines[0] = "MQTT " + piMonitorOledScopeLabel();

  String messageText = piMonitorLatestOledMessageText();
  messageText.replace('\r', '\n');
  messageText.replace('\n', ' ');

  int textOffset = 0;
  for (uint8_t i = 1; i < OLED_STATUS_LINE_COUNT; ++i) {
    lines[i] = messageText.substring(textOffset, textOffset + OLED_STATUS_MAX_CHARS);
    textOffset += OLED_STATUS_MAX_CHARS;
  }
}

void buildEnvironmentOledLines(String lines[OLED_STATUS_LINE_COUNT]) {
  lines[0] = "Environment";
  lines[1] = oledTemperatureLine();
  lines[2] = oledHumidityLine();
  lines[3] = oledPressureLine();
  lines[4] = envLogging ? String("Log: ") + envLogFileName
                        : environmentI2cPathLabel();
}

void buildVoiceMemoOledLines(String lines[OLED_STATUS_LINE_COUNT]) {
  lines[0] = "Voice Memos";
  lines[1] = oledVoiceMemoLine();
  lines[2] = selectedVoiceMemoNameForOled();
  lines[3] = voiceMemoRecording && voiceMemoMicStatus.length() > 0
                 ? voiceMemoMicStatus
                 : voiceMemoStatus.length() > 0 ? voiceMemoStatus
                                                : oledBatteryLine();
  lines[4] = voiceMemoRecording
                 ? String(voiceMemoRecordedBytes / 1024UL) + "KB P:" +
                       String(voiceMemoLastPeak)
                 : "OK play R rec";
}

void buildRtcStatusOledLines(String lines[OLED_STATUS_LINE_COUNT]) {
  lines[0] = "RTC";

  if (!rtcOnline) {
    lines[1] = "DS3231: missing";
    lines[2] = rtcI2cPathLabel();
    lines[3] = rtcStatus;
    lines[4] = "OK/R retry";
    return;
  }

  lines[1] = rtcOscillatorStopped ? "RTC: needs set" : "DS3231: online";
  lines[2] = rtcDateText();
  lines[3] = rtcTimeText();
  lines[4] = rtcOscillatorStopped
                 ? "N NTP S build"
                 : (rtcEepromDetected ? "N NTP EE:seen" : "N NTP EE:--");
}

void buildWifiOledLines(String lines[OLED_STATUS_LINE_COUNT]) {
  lines[0] = screenTitleForOled();
  lines[1] = oledWifiLine();

  if (currentScreen == Screen::WifiScan) {
    lines[2] = String("Networks: ") + String(wifiNetworkCount);
    if (wifiNetworkCount > 0 && selectedWifiIndex >= 0 &&
        selectedWifiIndex < wifiNetworkCount) {
      lines[3] = wifiNetworks[selectedWifiIndex].ssid;
    } else {
      lines[3] = "No networks";
    }
    lines[4] = "OK save R scan";
    return;
  }

  if (currentScreen == Screen::SavedWifi ||
      currentScreen == Screen::SavedWifiDeleteConfirm ||
      currentScreen == Screen::SavedWifiDeleteResult) {
    lines[2] = String("Saved: ") + String(savedWifiCount);
    if (savedWifiCount > 0 && selectedSavedWifiIndex >= 0 &&
        selectedSavedWifiIndex < savedWifiCount) {
      lines[3] = savedWifiNames[selectedSavedWifiIndex];
    } else {
      lines[3] = "No saved WiFi";
    }
    lines[4] = savedWifiDeleteResultMessage.length() > 0
                   ? savedWifiDeleteResultMessage
                   : "D delete";
    return;
  }

  lines[2] = wifiConnectSsid.length() > 0 ? wifiConnectSsid : "/config/wifi.txt";
  lines[3] = wifiConnectIp.length() > 0 ? wifiConnectIp : wifiConnectStatus;
  lines[4] = "OK retry D disc";
}

void buildLoraDiagOledLines(String lines[OLED_STATUS_LINE_COUNT]) {
  lines[0] = "LoRa Diag";
  lines[1] = loraIoExpanderDetected ? "Cap: LoRa-1262" : "Cap: not found";
  lines[2] = loraRadioReady
                 ? (loraListening ? "Radio: listening" : "Radio: ready")
                 : "Radio: not found";
  lines[3] = loraGnssLocationValid
                 ? String("GNSS Fix Sat:") + String(loraGnssSatellites)
                 : String("GNSS NoFix L:") + String(loraGnssLineCount);
  lines[4] = loraGnssLocationValid
                 ? String(loraGnssLatitude, 5) + "," +
                       String(loraGnssLongitude, 5)
                  : "RX only No TX";
}

void buildGnssDashboardOledLines(String lines[OLED_STATUS_LINE_COUNT]) {
  lines[0] = "GNSS Dash";
  lines[1] = loraGnssHasFreshFix()
                 ? String("Fix Sat:") + String(loraGnssSatellites)
                 : String("NoFix L:") + String(loraGnssLineCount);
  lines[2] = loraGnssLocationValid
                 ? String(loraGnssLatitude, 5) + "," +
                       String(loraGnssLongitude, 5)
                 : loraGnssStatus;
  lines[3] = String("HD:") + loraGnssHdopText() +
             String(" Sp:") + loraGnssSpeedText();
  lines[4] = loraGnssUtcText() + String(" A:") + loraGnssAltitudeText();
}

void buildGnssSkyViewOledLines(String lines[OLED_STATUS_LINE_COUNT]) {
  const GnssSkySatellite* satellite = selectedGnssSkySatellite();

  lines[0] = "GNSS Sky";
  if (satellite == nullptr) {
    lines[1] = "Sel: none";
    lines[2] = String("Sky:") + String(gnssSkySatelliteCount) + "/" +
               String(gnssSkySatellitesInView);
    lines[3] = loraGnssHasFreshFix()
                   ? String("Fix Sat:") + String(loraGnssSatellites)
                   : loraGnssStatus;
    lines[4] = String("GSV:") + String(gnssSkyGsvSentenceCount) +
               (lastGnssSkyGsvMs == 0
                    ? String(" wait")
                    : String(" ") +
                          String((millis() - lastGnssSkyGsvMs) / 1000UL) +
                          String("s"));
    return;
  }

  const String snr = gnssSkySatelliteSnrText(*satellite);
  lines[1] = String("Sel: ") + gnssSkySatelliteLabel(*satellite) + " " +
             gnssSkyConstellationName(satellite->constellation);
  lines[2] = String("PRN:") + String(satellite->prn) + String(" SNR:") + snr +
             (snr == "--" ? String("") : String("dB"));
  lines[3] = String("El:") + String(satellite->elevationDeg) +
             String(" Az:") + String(satellite->azimuthDeg);
  lines[4] = gnssSkyCompassDirection(satellite->azimuthDeg) +
             String(" age:") + gnssSkySatelliteAgeText(*satellite);
}

void buildReturnHomeOledLines(String lines[OLED_STATUS_LINE_COUNT]) {
  lines[0] = "Return Home";

  if (!returnHomeWaypointValid) {
    lines[1] = "Home: not saved";
    lines[2] = loraGnssHasFreshFix() ? "Fix: ready" : loraGnssStatus;
    lines[3] = String("Sat:") + loraGnssSatellitesText() +
               String(" HD:") + loraGnssHdopText();
    lines[4] = "S save home";
    return;
  }

  if (!returnHomeNavigationValid) {
    lines[1] = "Home: saved";
    lines[2] = returnHomeStatus;
    lines[3] = String("Sat:") + loraGnssSatellitesText() +
               String(" HD:") + loraGnssHdopText();
    lines[4] = "S upd D clr";
    return;
  }

  lines[1] = String("Dist: ") + returnHomeDistanceText();
  lines[2] = String("Bear: ") + returnHomeBearingText();
  lines[3] = returnHomeStatus;
  lines[4] = "S upd D clr";
}

void buildBreadcrumbLoggerOledLines(String lines[OLED_STATUS_LINE_COUNT]) {
  lines[0] = "Breadcrumbs";
  lines[1] = breadcrumbLogging
                 ? String("Log: ") + breadcrumbLogFileName
                 : "Log: stopped";
  lines[2] = loraGnssHasFreshFix()
                 ? String("Fix Sat:") + String(loraGnssSatellites)
                 : loraGnssStatus;
  lines[3] = breadcrumbLogging
                 ? String("Pts:") + String(breadcrumbLogSampleCount) +
                       String(" Miss:") + String(breadcrumbLogMissedFixCount)
                 : String("Sat:") + loraGnssSatellitesText() +
                       String(" HD:") + loraGnssHdopText();
  lines[4] = breadcrumbLogging ? "S stop CSV" : "S start CSV";
}

void buildLoraMessagesOledLines(String lines[OLED_STATUS_LINE_COUNT]) {
  lines[0] = "LoRa Messages";
  lines[1] = loraMessageRadioReady ? "Radio: listening" : "Radio: not found";
  lines[2] = String("TX:") + loraMessageTxCount + " ACK:" + loraMessageAckCount;
  lines[3] = String("RX:") + loraMessageRxCount + " TO:" + loraMessageTimeoutCount;
  lines[4] = loraMessageStatus;
}

void buildWifiConnectOledHelpLines(String lines[OLED_HELP_LINE_COUNT]) {
  lines[0] = "WiFi Connect";
  lines[1] = "Uses /config/wifi.txt";
  lines[2] = "Connects saved SSID";
  lines[3] = "OK retry connect";
  lines[4] = "D disconnect WiFi";
  lines[5] = "Back menu";
  lines[6] = wifiConnectSsid.length() > 0 ? "SSID: " + wifiConnectSsid
                                           : "SSID from SD config";
  lines[7] = wifiConnectIp.length() > 0 ? "IP: " + wifiConnectIp
                                        : wifiConnectStatus;
}

void buildPiMonitorOledHelpLines(String lines[OLED_HELP_LINE_COUNT]) {
  lines[0] = "Pi Monitor";
  lines[1] = "MQTT Pi status/cmds";
  lines[2] = "Arw choose project";
  lines[3] = "OK open/send/retry";
  lines[4] = "Back list/menu";
  lines[5] = "C read_now command";
  lines[6] = "T target I interval";
  lines[7] = "S msg page R reconn D off";
}

void buildSdManagerOledHelpLines(String lines[OLED_HELP_LINE_COUNT]) {
  lines[0] = "SD Manager";
  lines[1] = "Browse/edit microSD";
  lines[2] = "Arw move/scroll";
  lines[3] = "OK open/save/confirm";
  lines[4] = "Back up/cancel/delete";
  lines[5] = "R reload/rename";
  lines[6] = "I info N new D del";
  lines[7] = "E edit W/P templates";
}

void buildRtcStatusOledHelpLines(String lines[OLED_HELP_LINE_COUNT]) {
  lines[0] = "RTC";
  lines[1] = "DS3231 clock status";
  lines[2] = "N set from NTP";
  lines[3] = "S set build time";
  lines[4] = "OK/R retry read";
  lines[5] = "Back menu";
  lines[6] = "WiFi Connect for NTP";
  lines[7] = "PaHub ch5 RTC";
}

void buildGnssDashboardOledHelpLines(String lines[OLED_HELP_LINE_COUNT]) {
  lines[0] = "GNSS Dash";
  lines[1] = "GNSS fix/status view";
  lines[2] = "OK/R restart parser";
  lines[3] = "Back stop GNSS";
  lines[4] = "No LoRa radio/TX";
  lines[5] = "Sat/HDOP on LCD";
  lines[6] = "Lat/lon on LCD";
  lines[7] = "UTC/speed/alt LCD";
}

void buildGnssSkyViewOledHelpLines(String lines[OLED_HELP_LINE_COUNT]) {
  const GnssSkySatellite* satellite = selectedGnssSkySatellite();

  lines[0] = "Arw select satellite";
  lines[1] = "OK/R restart parser";
  lines[2] = "Back stop GNSS";

  if (satellite == nullptr) {
    lines[3] = "Sel: none";
    lines[4] = String("Sky:") + String(gnssSkySatelliteCount) + "/" +
               String(gnssSkySatellitesInView) + " GSV:" +
               String(gnssSkyGsvSentenceCount);
    lines[5] = loraGnssHasFreshFix()
                   ? String("Fix Sat:") + String(loraGnssSatellites)
                   : loraGnssStatus;
    lines[6] = lastGnssSkyGsvMs == 0
                   ? "Wait for GSV"
                   : String("GSV age:") +
                         String((millis() - lastGnssSkyGsvMs) / 1000UL) +
                         String("s");
    lines[7] = "No LoRa radio/TX";
    return;
  }

  const String snr = gnssSkySatelliteSnrText(*satellite);
  lines[3] = String("Sel: ") + gnssSkySatelliteLabel(*satellite) + " " +
             gnssSkyConstellationName(satellite->constellation);
  lines[4] = String("PRN:") + String(satellite->prn) + " SNR:" + snr +
             (snr == "--" ? String("") : String("dB"));
  lines[5] = String("El:") + String(satellite->elevationDeg) +
             " Az:" + String(satellite->azimuthDeg);
  lines[6] = String("Dir:") +
             gnssSkyCompassDirection(satellite->azimuthDeg) + " Age:" +
             gnssSkySatelliteAgeText(*satellite);
  lines[7] = String("Sky:") + String(gnssSkySatelliteCount) + "/" +
             String(gnssSkySatellitesInView);
}

void buildReturnHomeOledHelpLines(String lines[OLED_HELP_LINE_COUNT]) {
  lines[0] = "Return Home";
  lines[1] = "Guides to saved home";
  lines[2] = "S save/update fix";
  lines[3] = "D clear home";
  lines[4] = "OK/R restart GNSS";
  lines[5] = "Back stop GNSS";
  lines[6] = "Dist: " + returnHomeDistanceText();
  lines[7] = "Bear: " + returnHomeBearingText();
}

void buildBreadcrumbLoggerOledHelpLines(String lines[OLED_HELP_LINE_COUNT]) {
  lines[0] = "Breadcrumbs";
  lines[1] = "Logs GNSS CSV tracks";
  lines[2] = "S start/stop CSV";
  lines[3] = "OK/R restart GNSS";
  lines[4] = "Back closes log";
  lines[5] = "Fresh fix every 5s";
  lines[6] = String("Pts:") + String(breadcrumbLogSampleCount) +
             " Miss:" + String(breadcrumbLogMissedFixCount);
  lines[7] = breadcrumbLogFileName.length() > 0
                 ? "File: " + breadcrumbLogFileName
                 : "File: trackNNN.csv";
}

void buildLoraMessagesOledHelpLines(String lines[OLED_HELP_LINE_COUNT]) {
  lines[0] = "LoRa Messages";
  lines[1] = loraMessageComposing ? "Enter send message" : "N/OK new message";
  lines[2] = loraMessageComposing ? "Del erase character" : "Arrows browse";
  lines[3] = loraMessageComposing ? "Del empty cancels" : "R retry radio";
  lines[4] = loraMessageComposing ? "List Back: menu" : "Back sleep radio";
  lines[5] = String("TX:") + loraMessageTxCount + " ACK:" + loraMessageAckCount;
  lines[6] = String("RX:") + loraMessageRxCount + " TO:" + loraMessageTimeoutCount;
  lines[7] = loraMessageStatus;
}

void buildLoraDiagOledHelpLines(String lines[OLED_HELP_LINE_COUNT]) {
  lines[0] = "LoRa Diag";
  lines[1] = "RX packets + hardware";
  lines[2] = "Arrows change page";
  lines[3] = "C clear counters";
  lines[4] = "OK/R restart diag";
  lines[5] = "Back sleep radio";
  lines[6] = String("Pk:") + String(loraPacketCount) +
             " NMEA:" + String(loraGnssLineCount);
  lines[7] = String("RSSI:") +
             (loraPacketCount > 0 ? String(loraLastRssi, 1) : String("--"));
}

bool renderOledHelpDashboard() {
  String lines[OLED_HELP_LINE_COUNT];

  if (currentScreen == Screen::PiMonitor) {
    if (piMonitorView != PiMonitorView::ProjectList) {
      return false;
    }

    buildPiMonitorOledHelpLines(lines);
    renderOledHelpLines(lines);
    return true;
  }

  switch (currentScreen) {
    case Screen::WifiConnect:
      buildWifiConnectOledHelpLines(lines);
      break;
    case Screen::SdManager:
      buildSdManagerOledHelpLines(lines);
      break;
    case Screen::RtcStatus:
      buildRtcStatusOledHelpLines(lines);
      break;
    case Screen::GnssDashboard:
      buildGnssDashboardOledHelpLines(lines);
      break;
    case Screen::GnssSkyView:
      buildGnssSkyViewOledHelpLines(lines);
      break;
    case Screen::ReturnHome:
      buildReturnHomeOledHelpLines(lines);
      break;
    case Screen::BreadcrumbLogger:
      buildBreadcrumbLoggerOledHelpLines(lines);
      break;
    case Screen::LoraMessages:
      buildLoraMessagesOledHelpLines(lines);
      break;
    case Screen::LoraDiag:
      buildLoraDiagOledHelpLines(lines);
      break;
    default:
      return false;
  }

  renderOledHelpLines(lines);
  return true;
}

void buildOledDashboardLines(String lines[OLED_STATUS_LINE_COUNT]) {
  buildDefaultOledLines(lines);

  switch (currentScreen) {
    case Screen::MainMenu:
      buildMainMenuOledLines(lines);
      break;
    case Screen::WifiScan:
    case Screen::WifiSaveConfirm:
    case Screen::WifiSaveResult:
    case Screen::SavedWifi:
    case Screen::SavedWifiDeleteConfirm:
    case Screen::SavedWifiDeleteResult:
    case Screen::WifiConnect:
      buildWifiOledLines(lines);
      break;
    case Screen::PiMonitor:
      buildPiMonitorOledLines(lines);
      break;
    case Screen::VoiceMemos:
    case Screen::VoiceMemoDeleteConfirm:
    case Screen::VoiceMemoDeleteResult:
      buildVoiceMemoOledLines(lines);
      break;
    case Screen::Environment:
    case Screen::EnvironmentLogName:
      buildEnvironmentOledLines(lines);
      break;
    case Screen::RtcStatus:
      buildRtcStatusOledLines(lines);
      break;
    case Screen::LoraDiag:
      buildLoraDiagOledLines(lines);
      break;
    case Screen::GnssDashboard:
      buildGnssDashboardOledLines(lines);
      break;
    case Screen::GnssSkyView:
      buildGnssSkyViewOledLines(lines);
      break;
    case Screen::ReturnHome:
      buildReturnHomeOledLines(lines);
      break;
    case Screen::BreadcrumbLogger:
      buildBreadcrumbLoggerOledLines(lines);
      break;
    case Screen::LoraMessages:
      buildLoraMessagesOledLines(lines);
      break;
    case Screen::LevelTool:
      lines[4] = oledLevelLine();
      break;
    case Screen::BatteryInfo:
      lines[1] = oledBatteryLine();
      lines[4] = String("Charge: ") + inferredChargingText(lastChargingStatus);
      break;
    case Screen::SystemInfo:
      lines[4] = String("Heap: ") + String(ESP.getFreeHeap() / 1024U) + " KB";
      break;
    case Screen::OledTest:
      break;
  }
}

}  // namespace

void advancePiMonitorOledMessagePage() {
  if (currentScreen != Screen::PiMonitor) {
    return;
  }

  ++piMonitorOledPageIndex;
  renderOledStatusDashboard();
}

void showOledTest() {
  renderOledTest();
}

void renderOledTest() {
  lastOledRefreshMs = millis();

  if (!oledInitialized || !oledOnline) {
    if (initOledDisplay()) {
      drawOledTestPattern();
    }
  } else {
    drawOledTestPattern();
  }

  beginContentDraw();
  contentCanvas.println("OLED proof-of-life");
  contentCanvas.printf("Path: %s\n", oledI2cPathLabel().c_str());
  contentCanvas.printf("Addr: 0x%02X Bus:%luk\n", oledActiveAddress,
                       static_cast<unsigned long>(currentOledBusFrequency() /
                                                  1000UL));
  contentCanvas.printf("Status: %s\n", oledStatus.substring(0, 22).c_str());
  contentCanvas.printf("Scan: %s\n", oledScanSummary.substring(0, 21).c_str());
  contentCanvas.printf("Draws: %lu\n", oledDrawCount);
  contentCanvas.println("OK/R retry");
  contentCanvas.println("Back menu");
  commitContentDraw();
}

void serviceOledStatusDashboard() {
  const unsigned long now = millis();
  const uint32_t refreshInterval =
      currentScreen == Screen::OledTest ? OLED_TEST_REFRESH_INTERVAL_MS
                                        : OLED_STATUS_REFRESH_INTERVAL_MS;

  if (now - lastOledRefreshMs <= refreshInterval) {
    return;
  }

  if (currentScreen == Screen::OledTest) {
    renderOledTest();
  } else {
    renderOledStatusDashboard();
  }
}

void renderOledStatusDashboard() {
  lastOledRefreshMs = millis();

  if (!ensureOledReady()) {
    return;
  }

  if (!selectOledI2cPath()) {
    oledOnline = false;
    oledStatus = "PaHub select failed";
    return;
  }

  if (renderOledHelpDashboard()) {
    return;
  }

  if (currentScreen == Screen::PiMonitor) {
    renderPiMonitorOledMessages();
    return;
  }

  String lines[OLED_STATUS_LINE_COUNT];
  buildOledDashboardLines(lines);

  oledDrawCount++;
  oledDisplay.clearBuffer();
  setOledBusClock();
  oledDisplay.setFont(u8g2_font_6x10_tf);
  for (uint8_t i = 0; i < OLED_STATUS_LINE_COUNT; ++i) {
    drawOledLine(i, lines[i]);
  }
  oledDisplay.sendBuffer();
  restoreExternalI2cBusClock();
}

void setOledStatusLine(const String& line) {
  oledStatusLine = line.substring(0, OLED_STATUS_MAX_CHARS);
}

void clearOledStatusLine() {
  oledStatusLine = "";
}

bool initOledDisplay() {
  lastOledInitAttemptMs = millis();
  oledInitialized = false;
  oledOnline = false;
  oledActiveAddress = 0;
  oledActiveBusFrequency = 0;
  oledStatus = "Starting...";
  oledScanSummary = "Scanning...";

  const uint32_t probeFrequencies[] = {OLED_I2C_FAST_FREQUENCY,
                                       OLED_I2C_FALLBACK_FREQUENCY};
  for (uint8_t i = 0; i < 2; ++i) {
    if (tryOledProbeAtFrequency(probeFrequencies[i])) {
      break;
    }
  }

  if (oledActiveAddress == 0) {
    if (!i2cHubDetected) {
      oledStatus = "PaHub not found";
      oledScanSummary = "No PaHub";
      Serial.println(
          "OLED Test: PaHub not found; OLED must be on PaHub channel 1.");
    } else {
      oledStatus = "OLED not found";
      Serial.printf(
          "OLED Test: no display found at 0x%02X or 0x%02X on PaHub channel %u. %s\n",
          OLED_I2C_ADDRESS_PRIMARY, OLED_I2C_ADDRESS_SECONDARY,
          I2C_HUB_OLED_CHANNEL, oledScanSummary.c_str());
    }
    restoreExternalI2cBusClock();
    return false;
  }

  Wire.setClock(oledActiveBusFrequency);
  if (!selectOledI2cPath()) {
    oledStatus = "PaHub select failed";
    oledScanSummary = "PaHub select fail";
    restoreExternalI2cBusClock();
    return false;
  }

  oledDisplay.setI2CAddress(oledActiveAddress << 1);
  oledDisplay.setBusClock(oledActiveBusFrequency);
  oledDisplay.begin();
  oledDisplay.setBusClock(oledActiveBusFrequency);

  oledInitialized = true;
  oledOnline = true;
  oledStatus = String("Online ") + formatI2cAddress(oledActiveAddress) + " " +
               String(oledActiveBusFrequency / 1000UL) + "k";

  Serial.printf(
      "OLED Test: SSD1309 online at 0x%02X on PaHub channel %u at %lu kHz.\n",
      oledActiveAddress, I2C_HUB_OLED_CHANNEL,
      static_cast<unsigned long>(oledActiveBusFrequency / 1000UL));
  restoreExternalI2cBusClock();
  return true;
}

bool ensureOledReady() {
  if (oledInitialized && oledOnline) {
    return true;
  }

  if (millis() - lastOledInitAttemptMs < OLED_RETRY_INTERVAL_MS) {
    return false;
  }

  return initOledDisplay();
}

bool probeOledAddress(uint8_t address) {
  if (!selectOledI2cPath()) {
    return false;
  }

  return probeSelectedI2cAddress(address);
}

void drawOledTestPattern() {
  if (!oledInitialized || !oledOnline) {
    return;
  }

  if (!selectOledI2cPath()) {
    oledOnline = false;
    oledStatus = "PaHub select failed";
    return;
  }
  setOledBusClock();

  oledDrawCount++;

  oledDisplay.clearBuffer();
  oledDisplay.setFont(u8g2_font_6x10_tf);
  oledDisplay.drawStr(0, 10, "Scoober OLED");
  oledDisplay.drawStr(0, 24, "SSD1309 ch1");

  char statusLine[22];
  snprintf(statusLine, sizeof(statusLine), "0x%02X draw %lu",
           oledActiveAddress, oledDrawCount);
  oledDisplay.drawStr(0, 38, statusLine);

  const uint8_t markerX = 6 + ((oledDrawCount * 7) % 110);
  oledDisplay.drawFrame(0, 0, 128, 64);
  oledDisplay.drawBox(markerX, 52, 10, 6);
  oledDisplay.sendBuffer();
  restoreExternalI2cBusClock();
}
