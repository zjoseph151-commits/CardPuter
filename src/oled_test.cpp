#include "app.h"

namespace {

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
    case Screen::VoiceMemos:
    case Screen::VoiceMemoDeleteConfirm:
    case Screen::VoiceMemoDeleteResult:
      return "Voice Memos";
    case Screen::Environment:
    case Screen::EnvironmentLogName:
      return "Environment";
    case Screen::OledTest:
      return "OLED Test";
    case Screen::GnssDashboard:
      return "GNSS Dash";
    case Screen::GnssSkyView:
      return "GNSS Sky";
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

void drawOledLine(uint8_t row, const String& text) {
  if (row >= OLED_STATUS_LINE_COUNT) {
    return;
  }

  const uint8_t y = 10 + (row * 12);
  const String clipped = clippedOledText(text);
  oledDisplay.drawStr(0, y, clipped.c_str());
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

String activePiMonitorTargetForOled() {
  String target = piMonitorSelectedCommandTarget;
  target.trim();

  if (target.length() == 0) {
    target = piMonitorCommandTarget;
    target.trim();
  }

  return target.length() > 0 ? target : "none";
}

String oledPiCommandLine() {
  String command = piMonitorCommandStatus;
  command.trim();

  if (command.startsWith("read_now")) {
    return "Cmd: read_now";
  }

  if (command.startsWith("set ")) {
    return "Cmd: " + command.substring(0, 12);
  }

  if (command.length() == 0 || command == "No command sent.") {
    return "Cmd: ready";
  }

  return "Cmd: " + command;
}

int oledPiDeviceCount() {
  int count = 0;

  for (int i = 0; i < MAX_PI_MONITOR_DEVICES; ++i) {
    if (piMonitorDevices[i].active) {
      ++count;
    }
  }

  return count;
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
  lines[0] = "Pi Monitor";
  lines[1] = oledMqttLine();
  lines[2] = "Tgt: " + activePiMonitorTargetForOled();
  lines[3] = oledPiCommandLine();
  lines[4] = String("Msgs: ") + String(piMonitorMessageCount) + " Dev:" +
             String(oledPiDeviceCount());
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
  lines[3] = voiceMemoStatus.length() > 0 ? voiceMemoStatus : oledBatteryLine();
  lines[4] = voiceMemoRecording ? String(voiceMemoRecordedBytes / 1024UL) + " KB"
                                : "OK play R rec";
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
  refreshGnssSkySatellites();

  lines[0] = "GNSS Sky";
  lines[1] = String("Sky:") + String(gnssSkySatelliteCount) + "/" +
             String(gnssSkySatellitesInView);
  lines[2] = String("GSV:") + String(gnssSkyGsvSentenceCount) +
             (lastGnssSkyGsvMs == 0
                  ? String(" wait")
                  : String(" ") +
                        String((millis() - lastGnssSkyGsvMs) / 1000UL) +
                        String("s"));
  lines[3] = loraGnssHasFreshFix()
                 ? String("Fix Sat:") + String(loraGnssSatellites)
                 : loraGnssStatus;
  lines[4] = loraGnssUtcText();
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
    case Screen::LoraDiag:
      buildLoraDiagOledLines(lines);
      break;
    case Screen::GnssDashboard:
      buildGnssDashboardOledLines(lines);
      break;
    case Screen::GnssSkyView:
      buildGnssSkyViewOledLines(lines);
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
  contentCanvas.printf("Addr: 0x%02X\n", oledActiveAddress);
  contentCanvas.printf("Status: %s\n", oledStatus.substring(0, 22).c_str());
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

  String lines[OLED_STATUS_LINE_COUNT];
  buildOledDashboardLines(lines);

  oledDrawCount++;
  oledDisplay.clearBuffer();
  oledDisplay.setFont(u8g2_font_6x10_tf);
  for (uint8_t i = 0; i < OLED_STATUS_LINE_COUNT; ++i) {
    drawOledLine(i, lines[i]);
  }
  oledDisplay.sendBuffer();
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
  oledStatus = "Starting...";

  Wire.begin(ENV_I2C_SDA_PIN, ENV_I2C_SCL_PIN, ENV_I2C_FREQUENCY);
  detectI2cHub();

  if (!selectOledI2cPath()) {
    oledStatus = "PaHub not found";
    Serial.println("OLED Test: PaHub not found; OLED must be on PaHub channel 1.");
    return false;
  }

  if (probeOledAddress(OLED_I2C_ADDRESS_PRIMARY)) {
    oledActiveAddress = OLED_I2C_ADDRESS_PRIMARY;
  } else if (probeOledAddress(OLED_I2C_ADDRESS_SECONDARY)) {
    oledActiveAddress = OLED_I2C_ADDRESS_SECONDARY;
  } else {
    oledStatus = "OLED not found";
    Serial.println("OLED Test: no display found on PaHub channel 1.");
    return false;
  }

  if (!selectOledI2cPath()) {
    oledStatus = "PaHub select failed";
    return false;
  }

  oledDisplay.setI2CAddress(oledActiveAddress << 1);
  oledDisplay.setBusClock(ENV_I2C_FREQUENCY);
  oledDisplay.begin();

  oledInitialized = true;
  oledOnline = true;
  oledStatus = String("Online 0x") + String(oledActiveAddress, HEX);

  Serial.printf("OLED Test: SSD1309 online at 0x%02X on PaHub channel %u.\n",
                oledActiveAddress, I2C_HUB_OLED_CHANNEL);
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

  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
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
}
