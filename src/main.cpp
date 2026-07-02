#include <Arduino.h>
#include <M5Cardputer.h>
#include <M5UnitENV.h>
#include <Preferences.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <WiFi.h>
#include <math.h>

constexpr const char* FIRMWARE_NAME = "Scoober";
constexpr const char* FIRMWARE_VERSION = "v0.1.0";

constexpr int HEADER_HEIGHT = 22;
constexpr int CONTENT_TOP = HEADER_HEIGHT + 4;
constexpr int CHARGING_TREND_THRESHOLD_MV = 20;
constexpr int CHARGING_CONFIRM_SAMPLES = 3;
constexpr int CHARGING_CLEAR_SAMPLES = 3;
constexpr int MAX_WIFI_NETWORKS = 40;
constexpr int WIFI_VISIBLE_ROWS = 5;
constexpr int MAX_SAVED_WIFI_NAMES = 20;
constexpr int SAVED_WIFI_VISIBLE_ROWS = 5;
constexpr int SD_SPI_SCK_PIN = 40;
constexpr int SD_SPI_MISO_PIN = 39;
constexpr int SD_SPI_MOSI_PIN = 14;
constexpr int SD_SPI_CS_PIN = 12;
constexpr int SD_SPI_FREQUENCY = 25000000;
constexpr int ENV_I2C_SDA_PIN = 2;
constexpr int ENV_I2C_SCL_PIN = 1;
constexpr uint32_t ENV_I2C_FREQUENCY = 400000U;
constexpr uint32_t ENV_REFRESH_INTERVAL_MS = 1000;
constexpr uint32_t ENV_RETRY_INTERVAL_MS = 3000;
constexpr const char* VOICE_MEMO_DIR = "/memos";
constexpr int MAX_VOICE_MEMOS = 30;
constexpr int VOICE_MEMO_VISIBLE_ROWS = 5;
constexpr size_t VOICE_RECORD_SAMPLE_RATE = 16000;
constexpr size_t VOICE_RECORD_CHUNK_SAMPLES = 240;
constexpr uint32_t VOICE_RECORD_MAX_SECONDS = 30;
constexpr uint32_t VOICE_RECORD_MAX_BYTES =
    VOICE_RECORD_SAMPLE_RATE * VOICE_RECORD_MAX_SECONDS * sizeof(int16_t);
constexpr float LEVEL_DOT_SCALE_PIXELS = 70.0f;
constexpr int LEVEL_TOLERANCE_PIXELS = 6;
constexpr float LEVEL_SMOOTHING = 0.25f;
constexpr float LEVEL_RAD_TO_DEG = 57.2957795f;

enum class Screen {
  MainMenu,
  BatteryInfo,
  SystemInfo,
  WifiScan,
  WifiSaveConfirm,
  WifiSaveResult,
  SavedWifi,
  SavedWifiDeleteConfirm,
  SavedWifiDeleteResult,
  VoiceMemos,
  VoiceMemoDeleteConfirm,
  VoiceMemoDeleteResult,
  Environment,
  LevelTool,
};

struct MenuItem {
  const char* label;
  Screen screen;
};

struct BatteryTrend {
  bool initialized = false;
  int firstVoltageMv = 0;
  int lastVoltageMv = 0;
  int minVoltageMv = 0;
  int maxVoltageMv = 0;
  int firstLevel = -1;
  int lastLevel = -1;
  int risingSamples = 0;
  int fallingSamples = 0;
  int trendAboveThresholdSamples = 0;
  bool inferredCharging = false;
};

struct WifiNetwork {
  String ssid;
  int rssi = 0;
  int channel = 0;
  int encryption = 0;
};

struct VoiceMemoFile {
  String name;
  String path;
  uint32_t size = 0;
};

struct WavHeader {
  char riff[4] = {'R', 'I', 'F', 'F'};
  uint32_t fileSize = 0;
  char wave[4] = {'W', 'A', 'V', 'E'};
  char fmt[4] = {'f', 'm', 't', ' '};
  uint32_t fmtSize = 16;
  uint16_t audioFormat = 1;
  uint16_t numChannels = 1;
  uint32_t sampleRate = VOICE_RECORD_SAMPLE_RATE;
  uint32_t byteRate = VOICE_RECORD_SAMPLE_RATE * sizeof(int16_t);
  uint16_t blockAlign = sizeof(int16_t);
  uint16_t bitsPerSample = 16;
  char data[4] = {'d', 'a', 't', 'a'};
  uint32_t dataSize = 0;
};

constexpr MenuItem MENU_ITEMS[] = {
    {"Battery", Screen::BatteryInfo},
    {"System", Screen::SystemInfo},
    {"WiFi Scan", Screen::WifiScan},
    {"Saved WiFi", Screen::SavedWifi},
    {"Voice Memos", Screen::VoiceMemos},
    {"Environment", Screen::Environment},
    {"Level", Screen::LevelTool},
};
constexpr int MENU_ITEM_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);

SHT3X envSht30;
QMP6988 envQmp6988;
Screen currentScreen = Screen::MainMenu;
int selectedMenuIndex = 0;
unsigned long lastSystemRefreshMs = 0;
unsigned long lastLevelRefreshMs = 0;
unsigned long lastBatterySampleMs = 0;
unsigned long lastEnvironmentRefreshMs = 0;
unsigned long lastEnvironmentRetryMs = 0;
M5Canvas contentCanvas(&M5Cardputer.Display);
BatteryTrend batteryTrend;
WifiNetwork wifiNetworks[MAX_WIFI_NETWORKS];
int wifiNetworkCount = 0;
int selectedWifiIndex = 0;
int wifiScrollOffset = 0;
Preferences wifiPrefs;
String savedWifiNames[MAX_SAVED_WIFI_NAMES];
int savedWifiCount = 0;
int selectedSavedWifiIndex = 0;
int savedWifiScrollOffset = 0;
String wifiSaveMessage;
String wifiSaveResultSsid;
String pendingDeleteWifiName;
int pendingDeleteWifiIndex = -1;
String savedWifiDeleteResultName;
String savedWifiDeleteResultMessage;
VoiceMemoFile voiceMemos[MAX_VOICE_MEMOS];
int voiceMemoCount = 0;
int selectedVoiceMemoIndex = 0;
int voiceMemoScrollOffset = 0;
bool voiceSdInitialized = false;
bool voiceSdAvailable = false;
bool voiceMemoRecording = false;
bool voiceMemoPlaying = false;
bool envSensorInitialized = false;
bool envSht30Ready = false;
bool envQmp6988Ready = false;
bool envHasTempHumidity = false;
bool envHasPressure = false;
File voiceMemoFile;
String voiceMemoStatus;
String activeVoiceMemoName;
String activeVoiceMemoPath;
String pendingVoiceMemoDeleteName;
String pendingVoiceMemoDeletePath;
String voiceMemoDeleteResultMessage;
String envStatus = "Not initialized.";
uint32_t voiceMemoRecordedBytes = 0;
unsigned long voiceMemoRecordingStartedMs = 0;
unsigned long lastVoiceMemoRenderMs = 0;
int16_t voiceRecordBuffer[VOICE_RECORD_CHUNK_SAMPLES];
int lastBatteryVoltageMv = 0;
int lastBatteryLevel = -1;
int lastVbusVoltageMv = -1;
int lastBatteryCurrentMa = 0;
m5::Power_Class::is_charging_t lastChargingStatus = m5::Power_Class::charge_unknown;
float envTemperatureC = 0.0f;
float envHumidityPercent = 0.0f;
float envPressureHpa = 0.0f;
float envAltitudeM = 0.0f;
float smoothedLevelX = 0.0f;
float smoothedLevelY = 0.0f;
bool levelSmoothingInitialized = false;

void showMainMenu();
void showBatteryInfo();
void showSystemInfo();
void showWifiScan();
void renderWifiSaveConfirm();
void renderWifiSaveResult();
void renderSavedWifiList();
void renderSavedWifiDeleteConfirm();
void renderSavedWifiDeleteResult();
void showVoiceMemos();
void renderVoiceMemos();
void renderVoiceMemoDeleteConfirm();
void renderVoiceMemoDeleteResult();
void showEnvironment();
void showLevelTool();

void setScreen(Screen screen);
void drawHeader(const char* title);
void drawScreenFrame(const char* title);
void beginContentDraw();
void commitContentDraw();
void handleKeyboard();
void moveMenuSelection(int direction);
void activateSelectedMenuItem();
void scanWifiNetworks();
void renderWifiScan();
void moveWifiSelection(int direction);
void returnToWifiScan();
void loadSavedWifiNames();
bool persistSavedWifiNames();
bool savedWifiNameExists(const String& ssid);
bool saveWifiName(const String& ssid);
bool deleteSavedWifiName(int savedIndex);
void moveSavedWifiSelection(int direction);
bool initVoiceMemoSd();
void scanVoiceMemos();
bool findNextVoiceMemoPath(String& path, String& name);
void moveVoiceMemoSelection(int direction);
bool startVoiceMemoRecording();
void serviceVoiceMemoRecording();
void stopVoiceMemoRecording(const char* message);
void writeWavHeader(File& file, uint32_t dataBytes);
bool playSelectedVoiceMemo();
bool deleteSelectedVoiceMemo();
void resetVoiceMemoAudio();
bool initEnvironmentSensor();
bool readEnvironmentSensor();
void resetLevelSmoothing();
void drawLevelCrosshair(int centerX, int centerY);
void drawLevelDot(int dotX, int dotY, bool isLevel);
void printKeyState(const Keyboard_Class::KeysState& keys);
bool isMenuBackKey(const Keyboard_Class::KeysState& keys);
void sampleBatteryStatus();
void updateBatteryTrend(int voltageMv, int batteryLevel);
const char* chargingStatusText(m5::Power_Class::is_charging_t status);
const char* inferredChargingText(m5::Power_Class::is_charging_t status);

void setup() {
  Serial.begin(115200);
  delay(200);

  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setFont(&fonts::Font2);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(WHITE, BLACK);
  M5Cardputer.Display.fillScreen(BLACK);
  contentCanvas.setColorDepth(16);
  contentCanvas.createSprite(M5Cardputer.Display.width(),
                             M5Cardputer.Display.height() - CONTENT_TOP);
  contentCanvas.setFont(&fonts::Font2);
  contentCanvas.setTextSize(1);

  Serial.println();
  Serial.println(FIRMWARE_NAME);
  Serial.println(FIRMWARE_VERSION);
  loadSavedWifiNames();
  sampleBatteryStatus();

  drawHeader("M5 Cardputer Lab");
  M5Cardputer.Display.setCursor(8, 38);
  M5Cardputer.Display.println("Scoober");
  M5Cardputer.Display.println(FIRMWARE_VERSION);
  delay(1200);

  setScreen(Screen::MainMenu);
}

void loop() {
  M5Cardputer.update();
  handleKeyboard();

  const unsigned long now = millis();

  if (now - lastBatterySampleMs > 1000) {
    sampleBatteryStatus();
  }

  if ((currentScreen == Screen::BatteryInfo || currentScreen == Screen::SystemInfo) &&
      now - lastSystemRefreshMs > 1000) {
    if (currentScreen == Screen::BatteryInfo) {
      showBatteryInfo();
    } else {
      showSystemInfo();
    }
  }

  if (currentScreen == Screen::Environment &&
      now - lastEnvironmentRefreshMs > ENV_REFRESH_INTERVAL_MS) {
    showEnvironment();
  }

  if (currentScreen == Screen::LevelTool && now - lastLevelRefreshMs > 80) {
    showLevelTool();
  }

  if (currentScreen == Screen::VoiceMemos && voiceMemoRecording) {
    serviceVoiceMemoRecording();
  }

  delay(10);
}

void setScreen(Screen screen) {
  currentScreen = screen;
  lastSystemRefreshMs = 0;
  lastLevelRefreshMs = 0;
  lastEnvironmentRefreshMs = 0;

  switch (currentScreen) {
    case Screen::MainMenu:
      showMainMenu();
      break;
    case Screen::BatteryInfo:
      sampleBatteryStatus();
      drawScreenFrame("Battery");
      showBatteryInfo();
      break;
    case Screen::SystemInfo:
      drawScreenFrame("System");
      showSystemInfo();
      break;
    case Screen::WifiScan:
      drawScreenFrame("WiFi Scan");
      showWifiScan();
      break;
    case Screen::WifiSaveConfirm:
      wifiSaveMessage = "";
      drawScreenFrame("Save WiFi");
      renderWifiSaveConfirm();
      break;
    case Screen::WifiSaveResult:
      drawScreenFrame("Save WiFi");
      renderWifiSaveResult();
      break;
    case Screen::SavedWifi:
      drawScreenFrame("Saved WiFi");
      renderSavedWifiList();
      break;
    case Screen::SavedWifiDeleteConfirm:
      drawScreenFrame("Delete WiFi");
      renderSavedWifiDeleteConfirm();
      break;
    case Screen::SavedWifiDeleteResult:
      drawScreenFrame("Delete WiFi");
      renderSavedWifiDeleteResult();
      break;
    case Screen::VoiceMemos:
      drawScreenFrame("Voice Memos");
      showVoiceMemos();
      break;
    case Screen::VoiceMemoDeleteConfirm:
      drawScreenFrame("Delete Memo");
      renderVoiceMemoDeleteConfirm();
      break;
    case Screen::VoiceMemoDeleteResult:
      drawScreenFrame("Delete Memo");
      renderVoiceMemoDeleteResult();
      break;
    case Screen::Environment:
      drawScreenFrame("Environment");
      initEnvironmentSensor();
      showEnvironment();
      break;
    case Screen::LevelTool:
      resetLevelSmoothing();
      drawScreenFrame("Level");
      showLevelTool();
      break;
  }
}

void drawHeader(const char* title) {
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.fillRect(0, 0, M5Cardputer.Display.width(), HEADER_HEIGHT, NAVY);
  M5Cardputer.Display.setTextColor(WHITE, NAVY);
  M5Cardputer.Display.setCursor(6, 4);
  M5Cardputer.Display.print(title);
  M5Cardputer.Display.setTextColor(WHITE, BLACK);
}

void drawScreenFrame(const char* title) {
  drawHeader(title);
}

void beginContentDraw() {
  contentCanvas.fillScreen(BLACK);
  contentCanvas.setTextColor(WHITE, BLACK);
  contentCanvas.setCursor(8, 4);
}

void commitContentDraw() {
  contentCanvas.pushSprite(0, CONTENT_TOP);
}

void showMainMenu() {
  drawHeader("Scoober");
  M5Cardputer.Display.setCursor(8, 28);
  M5Cardputer.Display.setTextColor(LIGHTGREY, BLACK);
  M5Cardputer.Display.println("Use arrows, OK to select");

  for (int i = 0; i < MENU_ITEM_COUNT; ++i) {
    const int rowY = 38 + (i * 14);
    const bool selected = i == selectedMenuIndex;

    if (selected) {
      M5Cardputer.Display.fillRect(6, rowY - 2, M5Cardputer.Display.width() - 12, 14,
                                   DARKGREEN);
      M5Cardputer.Display.setTextColor(WHITE, DARKGREEN);
      M5Cardputer.Display.setCursor(12, rowY);
      M5Cardputer.Display.print("> ");
    } else {
      M5Cardputer.Display.setTextColor(WHITE, BLACK);
      M5Cardputer.Display.setCursor(22, rowY);
    }

    M5Cardputer.Display.print(MENU_ITEMS[i].label);
  }

  M5Cardputer.Display.setTextColor(WHITE, BLACK);
}

void showBatteryInfo() {
  lastSystemRefreshMs = millis();

  beginContentDraw();

  if (lastBatteryVoltageMv > 0) {
    contentCanvas.printf("Batt: %d mV", lastBatteryVoltageMv);
  } else {
    contentCanvas.print("Batt: unavailable");
  }

  if (lastBatteryLevel >= 0) {
    contentCanvas.printf(" %d%%\n", lastBatteryLevel);
  } else {
    contentCanvas.println(" --%");
  }

  contentCanvas.printf("Charge: %s\n", inferredChargingText(lastChargingStatus));
  contentCanvas.printf("API: %s\n", chargingStatusText(lastChargingStatus));

  if (lastVbusVoltageMv >= 0) {
    contentCanvas.printf("VBUS:%d mV ", lastVbusVoltageMv);
  } else {
    contentCanvas.print("VBUS:n/a ");
  }

  if (lastBatteryCurrentMa != 0) {
    contentCanvas.printf("I:%+dmA\n", lastBatteryCurrentMa);
  } else {
    contentCanvas.println("I:n/a");
  }

  if (batteryTrend.initialized && lastBatteryVoltageMv > 0) {
    contentCanvas.printf("Trend: %+d mV",
                         batteryTrend.lastVoltageMv - batteryTrend.minVoltageMv);
    if (lastBatteryLevel >= 0 && batteryTrend.firstLevel >= 0) {
      contentCanvas.printf(" %+d%%", batteryTrend.lastLevel - batteryTrend.firstLevel);
    }
    contentCanvas.println();
  }

  commitContentDraw();
}

void showSystemInfo() {
  lastSystemRefreshMs = millis();

  beginContentDraw();
  contentCanvas.printf("Firmware: %s\n", FIRMWARE_VERSION);
  contentCanvas.printf("Uptime: %lu sec\n", millis() / 1000UL);
  contentCanvas.printf("Free heap: %u\n", ESP.getFreeHeap());
  contentCanvas.printf("CPU: %u MHz\n", ESP.getCpuFreqMHz());
  contentCanvas.printf("Flash: %u MB\n", ESP.getFlashChipSize() / (1024U * 1024U));
  contentCanvas.printf("Screen: %d x %d\n", M5Cardputer.Display.width(),
                       M5Cardputer.Display.height());
  commitContentDraw();
}

void showWifiScan() {
  scanWifiNetworks();
  renderWifiScan();
}

void scanWifiNetworks() {
  beginContentDraw();
  contentCanvas.println("Scanning...");
  commitContentDraw();
  Serial.println("Starting WiFi scan...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(100);

  const int networkCount = WiFi.scanNetworks();
  wifiNetworkCount = constrain(networkCount, 0, MAX_WIFI_NETWORKS);
  selectedWifiIndex = 0;
  wifiScrollOffset = 0;

  if (networkCount <= 0) {
    Serial.println("No WiFi networks found.");
  } else {
    Serial.printf("Found %d WiFi network(s)\n", networkCount);

    for (int i = 0; i < wifiNetworkCount; ++i) {
      wifiNetworks[i].ssid = WiFi.SSID(i);
      wifiNetworks[i].rssi = WiFi.RSSI(i);
      wifiNetworks[i].channel = WiFi.channel(i);
      wifiNetworks[i].encryption = WiFi.encryptionType(i);

      Serial.printf("%2d: %s RSSI=%d CH=%d ENC=%d\n", i + 1,
                    wifiNetworks[i].ssid.c_str(), wifiNetworks[i].rssi,
                    wifiNetworks[i].channel, wifiNetworks[i].encryption);
    }
  }

  WiFi.scanDelete();
}

void renderWifiScan() {
  beginContentDraw();

  if (wifiNetworkCount <= 0) {
    contentCanvas.println("No networks found.");
    contentCanvas.println("R rescans.");
    commitContentDraw();
    return;
  }

  contentCanvas.printf("%d/%d OK save R scan\n", selectedWifiIndex + 1, wifiNetworkCount);

  const int shown = min(wifiNetworkCount, WIFI_VISIBLE_ROWS);
  for (int row = 0; row < shown; ++row) {
    const int networkIndex = wifiScrollOffset + row;
    if (networkIndex >= wifiNetworkCount) {
      break;
    }

    const bool selected = networkIndex == selectedWifiIndex;
    const int rowY = 20 + row * 17;

    if (selected) {
      contentCanvas.fillRect(4, rowY - 1, contentCanvas.width() - 8, 16, DARKGREEN);
      contentCanvas.setTextColor(WHITE, DARKGREEN);
    } else {
      contentCanvas.setTextColor(WHITE, BLACK);
    }

    String ssid = wifiNetworks[networkIndex].ssid;
    if (ssid.length() == 0) {
      ssid = "(hidden)";
    }

    contentCanvas.setCursor(8, rowY);
    contentCanvas.printf("%c%-14s %4d", selected ? '>' : ' ', ssid.substring(0, 14).c_str(),
                         wifiNetworks[networkIndex].rssi);
  }

  commitContentDraw();
}

void moveWifiSelection(int direction) {
  if (wifiNetworkCount <= 0) {
    return;
  }

  selectedWifiIndex += direction;
  if (selectedWifiIndex < 0) {
    selectedWifiIndex = wifiNetworkCount - 1;
  } else if (selectedWifiIndex >= wifiNetworkCount) {
    selectedWifiIndex = 0;
  }

  if (selectedWifiIndex < wifiScrollOffset) {
    wifiScrollOffset = selectedWifiIndex;
  } else if (selectedWifiIndex >= wifiScrollOffset + WIFI_VISIBLE_ROWS) {
    wifiScrollOffset = selectedWifiIndex - WIFI_VISIBLE_ROWS + 1;
  }

  renderWifiScan();
}

void renderWifiSaveConfirm() {
  beginContentDraw();

  if (wifiNetworkCount <= 0 || selectedWifiIndex < 0 ||
      selectedWifiIndex >= wifiNetworkCount) {
    contentCanvas.println("No network selected.");
    commitContentDraw();
    return;
  }

  String ssid = wifiNetworks[selectedWifiIndex].ssid;
  ssid.trim();

  if (ssid.length() == 0) {
    contentCanvas.println("Hidden SSID");
    contentCanvas.println("Cannot save name.");
    commitContentDraw();
    return;
  }

  contentCanvas.println("Save this network?");
  contentCanvas.println();
  contentCanvas.printf("%s\n", ssid.substring(0, 22).c_str());
  contentCanvas.println();
  contentCanvas.println("OK save");
  contentCanvas.println("Back cancel");

  if (wifiSaveMessage.length() > 0) {
    contentCanvas.println();
    contentCanvas.print(wifiSaveMessage);
  }

  commitContentDraw();
}

void renderWifiSaveResult() {
  beginContentDraw();

  contentCanvas.println(wifiSaveMessage);
  contentCanvas.println();

  if (wifiSaveResultSsid.length() > 0) {
    contentCanvas.println(wifiSaveResultSsid.substring(0, 22));
    contentCanvas.println();
  }

  contentCanvas.println("OK = WiFi Scan");
  commitContentDraw();
}

void renderSavedWifiList() {
  beginContentDraw();

  if (savedWifiCount <= 0) {
    contentCanvas.println("No saved networks.");
    contentCanvas.println("Save from WiFi Scan.");
    commitContentDraw();
    return;
  }

  contentCanvas.printf("%d saved %d/%d D delete\n", savedWifiCount,
                       selectedSavedWifiIndex + 1, savedWifiCount);

  const int shown = min(savedWifiCount, SAVED_WIFI_VISIBLE_ROWS);
  for (int row = 0; row < shown; ++row) {
    const int savedIndex = savedWifiScrollOffset + row;
    if (savedIndex >= savedWifiCount) {
      break;
    }

    const bool selected = savedIndex == selectedSavedWifiIndex;
    const int rowY = 24 + row * 17;

    if (selected) {
      contentCanvas.fillRect(4, rowY - 1, contentCanvas.width() - 8, 16, DARKGREEN);
      contentCanvas.setTextColor(WHITE, DARKGREEN);
    } else {
      contentCanvas.setTextColor(WHITE, BLACK);
    }

    contentCanvas.setCursor(8, rowY);
    contentCanvas.printf("%c%s", selected ? '>' : ' ',
                         savedWifiNames[savedIndex].substring(0, 22).c_str());
  }

  commitContentDraw();
}

void renderSavedWifiDeleteConfirm() {
  beginContentDraw();

  if (pendingDeleteWifiIndex < 0 || pendingDeleteWifiIndex >= savedWifiCount) {
    contentCanvas.println("No saved network.");
    commitContentDraw();
    return;
  }

  contentCanvas.println("Delete saved network?");
  contentCanvas.println();
  contentCanvas.println(pendingDeleteWifiName.substring(0, 22));
  contentCanvas.println();
  contentCanvas.println("OK delete");
  contentCanvas.println("Back cancel");
  commitContentDraw();
}

void renderSavedWifiDeleteResult() {
  beginContentDraw();

  contentCanvas.println(savedWifiDeleteResultMessage);
  contentCanvas.println();

  if (savedWifiDeleteResultName.length() > 0) {
    contentCanvas.println(savedWifiDeleteResultName.substring(0, 22));
    contentCanvas.println();
  }

  contentCanvas.println("OK = Saved WiFi");
  commitContentDraw();
}

void returnToWifiScan() {
  currentScreen = Screen::WifiScan;
  drawScreenFrame("WiFi Scan");
  renderWifiScan();
}

void loadSavedWifiNames() {
  savedWifiCount = 0;
  selectedSavedWifiIndex = 0;
  savedWifiScrollOffset = 0;

  if (!wifiPrefs.begin("scoober_wifi", true)) {
    Serial.println("Could not open saved WiFi preferences.");
    return;
  }

  const int storedCount =
      constrain(wifiPrefs.getInt("count", 0), 0, MAX_SAVED_WIFI_NAMES);

  for (int i = 0; i < storedCount; ++i) {
    char key[8];
    snprintf(key, sizeof(key), "ssid%02d", i);
    String ssid = wifiPrefs.getString(key, "");
    ssid.trim();

    if (ssid.length() > 0 && savedWifiCount < MAX_SAVED_WIFI_NAMES) {
      savedWifiNames[savedWifiCount++] = ssid;
    }
  }

  wifiPrefs.end();
  Serial.printf("Loaded %d saved WiFi name(s).\n", savedWifiCount);
}

bool persistSavedWifiNames() {
  if (!wifiPrefs.begin("scoober_wifi", false)) {
    Serial.println("Could not write saved WiFi preferences.");
    return false;
  }

  wifiPrefs.putInt("count", savedWifiCount);

  for (int i = 0; i < MAX_SAVED_WIFI_NAMES; ++i) {
    char key[8];
    snprintf(key, sizeof(key), "ssid%02d", i);

    if (i < savedWifiCount) {
      wifiPrefs.putString(key, savedWifiNames[i]);
    } else {
      wifiPrefs.remove(key);
    }
  }

  wifiPrefs.end();
  return true;
}

bool savedWifiNameExists(const String& ssid) {
  for (int i = 0; i < savedWifiCount; ++i) {
    if (savedWifiNames[i] == ssid) {
      return true;
    }
  }

  return false;
}

bool saveWifiName(const String& ssid) {
  String cleanSsid = ssid;
  cleanSsid.trim();

  if (cleanSsid.length() == 0) {
    wifiSaveMessage = "Cannot save hidden SSID.";
    return false;
  }

  if (savedWifiNameExists(cleanSsid)) {
    wifiSaveMessage = "Already saved.";
    return false;
  }

  if (savedWifiCount >= MAX_SAVED_WIFI_NAMES) {
    wifiSaveMessage = "Saved list is full.";
    return false;
  }

  savedWifiNames[savedWifiCount++] = cleanSsid;

  if (!persistSavedWifiNames()) {
    savedWifiCount--;
    wifiSaveMessage = "Save failed.";
    return false;
  }

  selectedSavedWifiIndex = savedWifiCount - 1;
  savedWifiScrollOffset = max(0, savedWifiCount - SAVED_WIFI_VISIBLE_ROWS);
  wifiSaveMessage = "Network saved";
  Serial.printf("Saved WiFi SSID: %s\n", cleanSsid.c_str());
  return true;
}

bool deleteSavedWifiName(int savedIndex) {
  if (savedIndex < 0 || savedIndex >= savedWifiCount) {
    savedWifiDeleteResultName = "";
    savedWifiDeleteResultMessage = "Delete failed";
    return false;
  }

  const String removedName = savedWifiNames[savedIndex];

  for (int i = savedIndex; i < savedWifiCount - 1; ++i) {
    savedWifiNames[i] = savedWifiNames[i + 1];
  }

  savedWifiNames[savedWifiCount - 1] = "";
  savedWifiCount--;

  if (!persistSavedWifiNames()) {
    loadSavedWifiNames();
    savedWifiDeleteResultName = removedName;
    savedWifiDeleteResultMessage = "Delete failed";
    return false;
  }

  if (savedWifiCount <= 0) {
    selectedSavedWifiIndex = 0;
    savedWifiScrollOffset = 0;
  } else {
    selectedSavedWifiIndex = min(selectedSavedWifiIndex, savedWifiCount - 1);
    savedWifiScrollOffset = min(savedWifiScrollOffset,
                                max(0, savedWifiCount - SAVED_WIFI_VISIBLE_ROWS));
  }

  savedWifiDeleteResultName = removedName;
  savedWifiDeleteResultMessage = "Network deleted";
  Serial.printf("Deleted saved WiFi SSID: %s\n", removedName.c_str());
  return true;
}

void moveSavedWifiSelection(int direction) {
  if (savedWifiCount <= 0) {
    return;
  }

  selectedSavedWifiIndex += direction;
  if (selectedSavedWifiIndex < 0) {
    selectedSavedWifiIndex = savedWifiCount - 1;
  } else if (selectedSavedWifiIndex >= savedWifiCount) {
    selectedSavedWifiIndex = 0;
  }

  if (selectedSavedWifiIndex < savedWifiScrollOffset) {
    savedWifiScrollOffset = selectedSavedWifiIndex;
  } else if (selectedSavedWifiIndex >= savedWifiScrollOffset + SAVED_WIFI_VISIBLE_ROWS) {
    savedWifiScrollOffset = selectedSavedWifiIndex - SAVED_WIFI_VISIBLE_ROWS + 1;
  }

  renderSavedWifiList();
}

void showVoiceMemos() {
  initVoiceMemoSd();
  scanVoiceMemos();
  renderVoiceMemos();
}

bool initVoiceMemoSd() {
  if (voiceSdInitialized && voiceSdAvailable) {
    return true;
  }

  voiceSdInitialized = true;
  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

  if (!SD.begin(SD_SPI_CS_PIN, SPI, SD_SPI_FREQUENCY)) {
    voiceSdAvailable = false;
    voiceMemoStatus = "SD init failed.";
    Serial.println("Voice memos: SD init failed.");
    return false;
  }

  if (SD.cardType() == CARD_NONE) {
    voiceSdAvailable = false;
    voiceMemoStatus = "No SD card.";
    Serial.println("Voice memos: no SD card.");
    return false;
  }

  if (!SD.exists(VOICE_MEMO_DIR) && !SD.mkdir(VOICE_MEMO_DIR)) {
    voiceSdAvailable = false;
    voiceMemoStatus = "Cannot make /memos.";
    Serial.println("Voice memos: could not create /memos.");
    return false;
  }

  voiceSdAvailable = true;
  voiceMemoStatus = "";
  return true;
}

void scanVoiceMemos() {
  if (!initVoiceMemoSd()) {
    voiceMemoCount = 0;
    selectedVoiceMemoIndex = 0;
    voiceMemoScrollOffset = 0;
    return;
  }

  const String selectedPath =
      (voiceMemoCount > 0 && selectedVoiceMemoIndex < voiceMemoCount)
          ? voiceMemos[selectedVoiceMemoIndex].path
          : "";

  voiceMemoCount = 0;
  File dir = SD.open(VOICE_MEMO_DIR);
  if (!dir || !dir.isDirectory()) {
    voiceMemoStatus = "Cannot open /memos.";
    if (dir) {
      dir.close();
    }
    return;
  }

  while (voiceMemoCount < MAX_VOICE_MEMOS) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;
    }

    String entryName = entry.name();
    String lowerName = entryName;
    lowerName.toLowerCase();

    if (!entry.isDirectory() && lowerName.endsWith(".wav")) {
      int slashIndex = entryName.lastIndexOf('/');
      String displayName = slashIndex >= 0 ? entryName.substring(slashIndex + 1) : entryName;
      String fullPath = entryName.startsWith("/") ? entryName : String(VOICE_MEMO_DIR) + "/" + entryName;

      voiceMemos[voiceMemoCount].name = displayName;
      voiceMemos[voiceMemoCount].path = fullPath;
      voiceMemos[voiceMemoCount].size = entry.size();
      voiceMemoCount++;
    }

    entry.close();
  }

  dir.close();

  selectedVoiceMemoIndex = 0;
  if (selectedPath.length() > 0) {
    for (int i = 0; i < voiceMemoCount; ++i) {
      if (voiceMemos[i].path == selectedPath) {
        selectedVoiceMemoIndex = i;
        break;
      }
    }
  }

  if (selectedVoiceMemoIndex >= voiceMemoCount) {
    selectedVoiceMemoIndex = max(0, voiceMemoCount - 1);
  }

  voiceMemoScrollOffset = min(voiceMemoScrollOffset,
                              max(0, voiceMemoCount - VOICE_MEMO_VISIBLE_ROWS));
}

bool findNextVoiceMemoPath(String& path, String& name) {
  if (!initVoiceMemoSd()) {
    return false;
  }

  for (int i = 1; i <= 999; ++i) {
    char filename[16];
    snprintf(filename, sizeof(filename), "memo%03d.wav", i);
    name = filename;
    path = String(VOICE_MEMO_DIR) + "/" + name;

    if (!SD.exists(path.c_str())) {
      return true;
    }
  }

  voiceMemoStatus = "Memo list full.";
  return false;
}

void renderVoiceMemos() {
  beginContentDraw();

  if (!voiceSdAvailable) {
    contentCanvas.println("SD card required.");
    contentCanvas.println(voiceMemoStatus);
    commitContentDraw();
    return;
  }

  if (voiceMemoRecording) {
    const uint32_t elapsedSeconds = (millis() - voiceMemoRecordingStartedMs) / 1000UL;
    contentCanvas.println("Recording...");
    contentCanvas.printf("%s\n", activeVoiceMemoName.c_str());
    contentCanvas.printf("%lu/%lu sec\n", elapsedSeconds, VOICE_RECORD_MAX_SECONDS);
    contentCanvas.printf("%lu KB\n", voiceMemoRecordedBytes / 1024UL);
    contentCanvas.println("R stop/save");
    commitContentDraw();
    return;
  }

  if (voiceMemoStatus.length() > 0) {
    contentCanvas.println(voiceMemoStatus.substring(0, 28));
  } else {
    contentCanvas.println("R record  OK play");
  }

  if (voiceMemoCount <= 0) {
    contentCanvas.println();
    contentCanvas.println("No memos yet.");
    commitContentDraw();
    return;
  }

  contentCanvas.printf("%d memo(s) %d/%d D del\n", voiceMemoCount,
                       selectedVoiceMemoIndex + 1, voiceMemoCount);

  const int shown = min(voiceMemoCount, VOICE_MEMO_VISIBLE_ROWS);
  for (int row = 0; row < shown; ++row) {
    const int memoIndex = voiceMemoScrollOffset + row;
    if (memoIndex >= voiceMemoCount) {
      break;
    }

    const bool selected = memoIndex == selectedVoiceMemoIndex;
    const int rowY = 38 + row * 16;

    if (selected) {
      contentCanvas.fillRect(4, rowY - 1, contentCanvas.width() - 8, 15, DARKGREEN);
      contentCanvas.setTextColor(WHITE, DARKGREEN);
    } else {
      contentCanvas.setTextColor(WHITE, BLACK);
    }

    contentCanvas.setCursor(8, rowY);
    contentCanvas.printf("%c%-14s %3luK", selected ? '>' : ' ',
                         voiceMemos[memoIndex].name.substring(0, 14).c_str(),
                         voiceMemos[memoIndex].size / 1024UL);
  }

  commitContentDraw();
}

void renderVoiceMemoDeleteConfirm() {
  beginContentDraw();
  contentCanvas.println("Delete voice memo?");
  contentCanvas.println();
  contentCanvas.println(pendingVoiceMemoDeleteName.substring(0, 22));
  contentCanvas.println();
  contentCanvas.println("OK delete");
  contentCanvas.println("Back cancel");
  commitContentDraw();
}

void renderVoiceMemoDeleteResult() {
  beginContentDraw();
  contentCanvas.println(voiceMemoDeleteResultMessage);
  contentCanvas.println();
  contentCanvas.println(pendingVoiceMemoDeleteName.substring(0, 22));
  contentCanvas.println();
  contentCanvas.println("OK = Voice Memos");
  commitContentDraw();
}

void moveVoiceMemoSelection(int direction) {
  if (voiceMemoCount <= 0) {
    return;
  }

  selectedVoiceMemoIndex += direction;
  if (selectedVoiceMemoIndex < 0) {
    selectedVoiceMemoIndex = voiceMemoCount - 1;
  } else if (selectedVoiceMemoIndex >= voiceMemoCount) {
    selectedVoiceMemoIndex = 0;
  }

  if (selectedVoiceMemoIndex < voiceMemoScrollOffset) {
    voiceMemoScrollOffset = selectedVoiceMemoIndex;
  } else if (selectedVoiceMemoIndex >= voiceMemoScrollOffset + VOICE_MEMO_VISIBLE_ROWS) {
    voiceMemoScrollOffset = selectedVoiceMemoIndex - VOICE_MEMO_VISIBLE_ROWS + 1;
  }

  renderVoiceMemos();
}

void writeWavHeader(File& file, uint32_t dataBytes) {
  WavHeader header;
  header.fileSize = 36 + dataBytes;
  header.dataSize = dataBytes;
  file.seek(0);
  file.write(reinterpret_cast<const uint8_t*>(&header), sizeof(WavHeader));
}

bool startVoiceMemoRecording() {
  if (voiceMemoRecording) {
    return true;
  }

  if (!findNextVoiceMemoPath(activeVoiceMemoPath, activeVoiceMemoName)) {
    renderVoiceMemos();
    return false;
  }

  voiceMemoFile = SD.open(activeVoiceMemoPath.c_str(), FILE_WRITE);
  if (!voiceMemoFile) {
    voiceMemoStatus = "File open failed.";
    renderVoiceMemos();
    return false;
  }

  writeWavHeader(voiceMemoFile, 0);
  voiceMemoRecordedBytes = 0;
  voiceMemoRecordingStartedMs = millis();
  lastVoiceMemoRenderMs = 0;

  M5Cardputer.Speaker.end();
  if (!M5Cardputer.Mic.isEnabled()) {
    M5Cardputer.Mic.begin();
  }

  if (!M5Cardputer.Mic.isEnabled()) {
    voiceMemoFile.close();
    SD.remove(activeVoiceMemoPath.c_str());
    voiceMemoStatus = "Mic unavailable.";
    renderVoiceMemos();
    return false;
  }

  voiceMemoRecording = true;
  voiceMemoStatus = "";
  Serial.printf("Recording voice memo: %s\n", activeVoiceMemoPath.c_str());
  renderVoiceMemos();
  return true;
}

void serviceVoiceMemoRecording() {
  if (!voiceMemoRecording) {
    return;
  }

  if (voiceMemoRecordedBytes >= VOICE_RECORD_MAX_BYTES) {
    stopVoiceMemoRecording("Saved");
    return;
  }

  if (!M5Cardputer.Mic.record(voiceRecordBuffer, VOICE_RECORD_CHUNK_SAMPLES,
                              VOICE_RECORD_SAMPLE_RATE)) {
    return;
  }

  size_t bytesToWrite = VOICE_RECORD_CHUNK_SAMPLES * sizeof(int16_t);
  const uint32_t remainingBytes = VOICE_RECORD_MAX_BYTES - voiceMemoRecordedBytes;
  if (bytesToWrite > remainingBytes) {
    bytesToWrite = remainingBytes;
  }

  const size_t written = voiceMemoFile.write(
      reinterpret_cast<const uint8_t*>(voiceRecordBuffer), bytesToWrite);
  if (written != bytesToWrite) {
    stopVoiceMemoRecording("Write failed");
    return;
  }

  voiceMemoRecordedBytes += written;

  if (voiceMemoRecordedBytes >= VOICE_RECORD_MAX_BYTES) {
    stopVoiceMemoRecording("Saved");
    return;
  }

  if (millis() - lastVoiceMemoRenderMs > 250) {
    lastVoiceMemoRenderMs = millis();
    renderVoiceMemos();
  }
}

void stopVoiceMemoRecording(const char* message) {
  if (!voiceMemoRecording) {
    return;
  }

  while (M5Cardputer.Mic.isRecording()) {
    delay(1);
  }

  voiceMemoRecording = false;
  M5Cardputer.Mic.end();

  if (voiceMemoFile) {
    if (voiceMemoRecordedBytes > 0) {
      writeWavHeader(voiceMemoFile, voiceMemoRecordedBytes);
    }
    voiceMemoFile.close();
  }

  if (voiceMemoRecordedBytes == 0) {
    SD.remove(activeVoiceMemoPath.c_str());
    voiceMemoStatus = "No audio saved.";
  } else {
    voiceMemoStatus = String(message) + ": " + activeVoiceMemoName;
    Serial.printf("Saved voice memo: %s (%lu bytes)\n", activeVoiceMemoPath.c_str(),
                  voiceMemoRecordedBytes);
  }

  scanVoiceMemos();
  for (int i = 0; i < voiceMemoCount; ++i) {
    if (voiceMemos[i].path == activeVoiceMemoPath) {
      selectedVoiceMemoIndex = i;
      break;
    }
  }
  renderVoiceMemos();
}

bool playSelectedVoiceMemo() {
  if (voiceMemoRecording || voiceMemoCount <= 0 ||
      selectedVoiceMemoIndex >= voiceMemoCount) {
    return false;
  }

  const String path = voiceMemos[selectedVoiceMemoIndex].path;
  File file = SD.open(path.c_str(), FILE_READ);
  if (!file) {
    voiceMemoStatus = "Open failed.";
    renderVoiceMemos();
    return false;
  }

  WavHeader header;
  if (file.read(reinterpret_cast<uint8_t*>(&header), sizeof(WavHeader)) !=
      sizeof(WavHeader)) {
    file.close();
    voiceMemoStatus = "Bad WAV file.";
    renderVoiceMemos();
    return false;
  }

  if (memcmp(header.riff, "RIFF", 4) != 0 || memcmp(header.wave, "WAVE", 4) != 0 ||
      header.audioFormat != 1 || header.bitsPerSample != 16 ||
      header.numChannels != 1) {
    file.close();
    voiceMemoStatus = "Unsupported WAV.";
    renderVoiceMemos();
    return false;
  }

  voiceMemoPlaying = true;
  voiceMemoStatus = "Playing: " + voiceMemos[selectedVoiceMemoIndex].name;
  renderVoiceMemos();

  M5Cardputer.Mic.end();
  M5Cardputer.Speaker.begin();
  M5Cardputer.Speaker.setVolume(180);

  uint32_t bytesRemaining = header.dataSize;
  while (bytesRemaining > 0) {
    size_t bytesToRead = sizeof(voiceRecordBuffer);
    if (bytesToRead > bytesRemaining) {
      bytesToRead = bytesRemaining;
    }

    const size_t bytesRead =
        file.read(reinterpret_cast<uint8_t*>(voiceRecordBuffer), bytesToRead);
    if (bytesRead == 0) {
      break;
    }

    M5Cardputer.Speaker.playRaw(voiceRecordBuffer, bytesRead / sizeof(int16_t),
                                header.sampleRate);
    while (M5Cardputer.Speaker.isPlaying()) {
      M5Cardputer.update();
      delay(1);
    }

    bytesRemaining -= bytesRead;
  }

  file.close();
  M5Cardputer.Speaker.end();
  voiceMemoPlaying = false;
  voiceMemoStatus = "Playback done.";
  renderVoiceMemos();
  return true;
}

bool deleteSelectedVoiceMemo() {
  if (pendingVoiceMemoDeletePath.length() == 0) {
    voiceMemoDeleteResultMessage = "Delete failed";
    return false;
  }

  if (SD.remove(pendingVoiceMemoDeletePath.c_str())) {
    voiceMemoDeleteResultMessage = "Memo deleted";
    Serial.printf("Deleted voice memo: %s\n", pendingVoiceMemoDeletePath.c_str());
    scanVoiceMemos();
    return true;
  }

  voiceMemoDeleteResultMessage = "Delete failed";
  return false;
}

void resetVoiceMemoAudio() {
  if (voiceMemoRecording) {
    stopVoiceMemoRecording("Saved");
  }
  M5Cardputer.Speaker.end();
  M5Cardputer.Mic.end();
}

void showEnvironment() {
  lastEnvironmentRefreshMs = millis();
  const bool hasFreshReading = readEnvironmentSensor();

  beginContentDraw();
  contentCanvas.println("ENV III Unit");

  if (!envSensorInitialized) {
    contentCanvas.println("ENV III not found");
    contentCanvas.println("Plug into Grove port.");
    contentCanvas.printf("SDA:G%d SCL:G%d\n", ENV_I2C_SDA_PIN, ENV_I2C_SCL_PIN);
    contentCanvas.println("Retrying...");
    commitContentDraw();
    return;
  }

  contentCanvas.printf("Status: %s\n", envStatus.c_str());

  if (envHasTempHumidity) {
    const float tempF = (envTemperatureC * 9.0f / 5.0f) + 32.0f;
    contentCanvas.printf("Temp: %.1f C / %.1f F\n", envTemperatureC, tempF);
    contentCanvas.printf("Humidity: %.1f %%\n", envHumidityPercent);
  } else {
    contentCanvas.println("Temp: waiting");
    contentCanvas.println("Humidity: waiting");
  }

  if (envHasPressure) {
    contentCanvas.printf("Pressure: %.1f hPa\n", envPressureHpa);
    contentCanvas.printf("Altitude: %.1f m\n", envAltitudeM);
  } else {
    contentCanvas.println("Pressure: waiting");
  }

  if (!hasFreshReading) {
    contentCanvas.println("Waiting for update...");
  }

  commitContentDraw();
}

bool initEnvironmentSensor() {
  lastEnvironmentRetryMs = millis();
  Wire.begin(ENV_I2C_SDA_PIN, ENV_I2C_SCL_PIN, ENV_I2C_FREQUENCY);

  envSht30Ready =
      envSht30.begin(&Wire, SHT3X_I2C_ADDR, ENV_I2C_SDA_PIN, ENV_I2C_SCL_PIN,
                     ENV_I2C_FREQUENCY);
  envQmp6988Ready =
      envQmp6988.begin(&Wire, QMP6988_SLAVE_ADDRESS_L, ENV_I2C_SDA_PIN,
                       ENV_I2C_SCL_PIN, ENV_I2C_FREQUENCY);
  envSensorInitialized = envSht30Ready || envQmp6988Ready;

  if (!envSensorInitialized) {
    envHasTempHumidity = false;
    envHasPressure = false;
    envStatus = "ENV III not found";
    Serial.println("Environment: ENV III not found on Grove I2C.");
    return false;
  }

  if (envSht30Ready && envQmp6988Ready) {
    envStatus = "Connected";
  } else if (envSht30Ready) {
    envStatus = "SHT30 only";
  } else {
    envStatus = "QMP6988 only";
  }

  Serial.printf("Environment: %s (SHT30=%s QMP6988=%s)\n", envStatus.c_str(),
                envSht30Ready ? "ok" : "missing",
                envQmp6988Ready ? "ok" : "missing");
  return true;
}

bool readEnvironmentSensor() {
  if (!envSensorInitialized) {
    if (millis() - lastEnvironmentRetryMs > ENV_RETRY_INTERVAL_MS) {
      initEnvironmentSensor();
    }
    return false;
  }

  bool updated = false;

  if (envSht30Ready && envSht30.update()) {
    envTemperatureC = envSht30.cTemp;
    envHumidityPercent = envSht30.humidity;
    envHasTempHumidity = true;
    updated = true;
  }

  if (envQmp6988Ready && envQmp6988.update()) {
    envPressureHpa = envQmp6988.pressure * 0.01f;
    envAltitudeM = envQmp6988.altitude;
    envHasPressure = true;
    updated = true;
  }

  if (updated) {
    envStatus = "Connected";
    Serial.printf("Environment: %.1f C %.1f %% %.1f hPa\n", envTemperatureC,
                  envHumidityPercent, envPressureHpa);
  } else if (millis() - lastEnvironmentRetryMs > ENV_RETRY_INTERVAL_MS) {
    initEnvironmentSensor();
  }

  return updated;
}

void showLevelTool() {
  lastLevelRefreshMs = millis();

  beginContentDraw();

  M5.Imu.update();

  if (!M5.Imu.isEnabled()) {
    contentCanvas.println("IMU unavailable.");
    contentCanvas.println("Cardputer-Adv only.");
    commitContentDraw();
    return;
  }

  m5::imu_data_t imuData = M5.Imu.getImuData();
  const float rawLevelX = imuData.accel.x;
  const float rawLevelY = imuData.accel.y;

  if (!levelSmoothingInitialized) {
    smoothedLevelX = rawLevelX;
    smoothedLevelY = rawLevelY;
    levelSmoothingInitialized = true;
  } else {
    smoothedLevelX += (rawLevelX - smoothedLevelX) * LEVEL_SMOOTHING;
    smoothedLevelY += (rawLevelY - smoothedLevelY) * LEVEL_SMOOTHING;
  }

  const int centerX = contentCanvas.width() / 2;
  const int centerY = contentCanvas.height() / 2;
  const int dotX = constrain(centerX + static_cast<int>(smoothedLevelX * LEVEL_DOT_SCALE_PIXELS),
                             8, contentCanvas.width() - 8);
  const int dotY = constrain(centerY + static_cast<int>(smoothedLevelY * LEVEL_DOT_SCALE_PIXELS),
                             8, contentCanvas.height() - 8);
  const int dx = dotX - centerX;
  const int dy = dotY - centerY;
  const bool isLevel = (dx * dx + dy * dy) <=
                       (LEVEL_TOLERANCE_PIXELS * LEVEL_TOLERANCE_PIXELS);

  const float pitchDeg =
      atan2f(-imuData.accel.x,
             sqrtf((imuData.accel.y * imuData.accel.y) +
                   (imuData.accel.z * imuData.accel.z))) *
      LEVEL_RAD_TO_DEG;
  const float rollDeg = atan2f(imuData.accel.y, imuData.accel.z) * LEVEL_RAD_TO_DEG;

  drawLevelCrosshair(centerX, centerY);
  drawLevelDot(dotX, dotY, isLevel);

  contentCanvas.setTextColor(isLevel ? GREEN : YELLOW, BLACK);
  contentCanvas.setCursor(8, 4);
  contentCanvas.print(isLevel ? "LEVEL" : "TILT");
  contentCanvas.setTextColor(WHITE, BLACK);
  contentCanvas.setCursor(8, contentCanvas.height() - 16);
  contentCanvas.printf("P:%+.1f R:%+.1f", pitchDeg, rollDeg);
  commitContentDraw();
}

void resetLevelSmoothing() {
  smoothedLevelX = 0.0f;
  smoothedLevelY = 0.0f;
  levelSmoothingInitialized = false;
}

void drawLevelCrosshair(int centerX, int centerY) {
  contentCanvas.drawCircle(centerX, centerY, 42, DARKGREY);
  contentCanvas.drawCircle(centerX, centerY, LEVEL_TOLERANCE_PIXELS, GREEN);
  contentCanvas.drawLine(centerX - 52, centerY, centerX + 52, centerY, DARKGREY);
  contentCanvas.drawLine(centerX, centerY - 42, centerX, centerY + 42, DARKGREY);
  contentCanvas.drawLine(centerX - 6, centerY, centerX + 6, centerY, GREEN);
  contentCanvas.drawLine(centerX, centerY - 6, centerX, centerY + 6, GREEN);
}

void drawLevelDot(int dotX, int dotY, bool isLevel) {
  const uint16_t color = isLevel ? GREEN : ORANGE;
  contentCanvas.fillCircle(dotX, dotY, 5, color);
  contentCanvas.drawCircle(dotX, dotY, 6, WHITE);
}

void handleKeyboard() {
  if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) {
    return;
  }

  Keyboard_Class::KeysState keys = M5Cardputer.Keyboard.keysState();
  printKeyState(keys);

  if (currentScreen != Screen::MainMenu && isMenuBackKey(keys)) {
    if (currentScreen == Screen::WifiSaveConfirm) {
      returnToWifiScan();
    } else if (currentScreen == Screen::SavedWifiDeleteConfirm) {
      setScreen(Screen::SavedWifi);
    } else if (currentScreen == Screen::VoiceMemoDeleteConfirm) {
      setScreen(Screen::VoiceMemos);
    } else if (currentScreen == Screen::VoiceMemos && voiceMemoRecording) {
      stopVoiceMemoRecording("Saved");
    } else {
      setScreen(Screen::MainMenu);
    }
    return;
  }

  if (currentScreen == Screen::MainMenu) {
    for (char key : keys.word) {
      if (key == ';' || key == ',') {
        moveMenuSelection(-1);
      } else if (key == '.' || key == '/') {
        moveMenuSelection(1);
      }
    }

    if (keys.enter) {
      activateSelectedMenuItem();
    }
  } else if (currentScreen == Screen::WifiScan) {
    for (char key : keys.word) {
      if (key == ';' || key == ',') {
        moveWifiSelection(-1);
      } else if (key == '.' || key == '/') {
        moveWifiSelection(1);
      } else if (key == 'r' || key == 'R') {
        scanWifiNetworks();
        renderWifiScan();
      }
    }

    if (keys.enter && wifiNetworkCount > 0) {
      setScreen(Screen::WifiSaveConfirm);
    }
  } else if (currentScreen == Screen::WifiSaveConfirm) {
    if (keys.enter && wifiNetworkCount > 0 && selectedWifiIndex < wifiNetworkCount) {
      wifiSaveResultSsid = wifiNetworks[selectedWifiIndex].ssid;
      wifiSaveResultSsid.trim();
      saveWifiName(wifiNetworks[selectedWifiIndex].ssid);
      setScreen(Screen::WifiSaveResult);
    }
  } else if (currentScreen == Screen::WifiSaveResult) {
    if (keys.enter) {
      returnToWifiScan();
    }
  } else if (currentScreen == Screen::SavedWifi) {
    for (char key : keys.word) {
      if (key == ';' || key == ',') {
        moveSavedWifiSelection(-1);
      } else if (key == '.' || key == '/') {
        moveSavedWifiSelection(1);
      } else if ((key == 'd' || key == 'D') && savedWifiCount > 0) {
        pendingDeleteWifiIndex = selectedSavedWifiIndex;
        pendingDeleteWifiName = savedWifiNames[selectedSavedWifiIndex];
        setScreen(Screen::SavedWifiDeleteConfirm);
      }
    }
  } else if (currentScreen == Screen::SavedWifiDeleteConfirm) {
    if (keys.enter) {
      deleteSavedWifiName(pendingDeleteWifiIndex);
      pendingDeleteWifiIndex = -1;
      pendingDeleteWifiName = "";
      setScreen(Screen::SavedWifiDeleteResult);
    }
  } else if (currentScreen == Screen::SavedWifiDeleteResult) {
    if (keys.enter) {
      setScreen(Screen::SavedWifi);
    }
  } else if (currentScreen == Screen::VoiceMemos) {
    if (voiceMemoRecording) {
      for (char key : keys.word) {
        if (key == 'r' || key == 'R') {
          stopVoiceMemoRecording("Saved");
        }
      }
      return;
    }

    for (char key : keys.word) {
      if (key == ';' || key == ',') {
        moveVoiceMemoSelection(-1);
      } else if (key == '.' || key == '/') {
        moveVoiceMemoSelection(1);
      } else if (key == 'r' || key == 'R') {
        startVoiceMemoRecording();
      } else if ((key == 'd' || key == 'D') && voiceMemoCount > 0) {
        pendingVoiceMemoDeleteName = voiceMemos[selectedVoiceMemoIndex].name;
        pendingVoiceMemoDeletePath = voiceMemos[selectedVoiceMemoIndex].path;
        setScreen(Screen::VoiceMemoDeleteConfirm);
      }
    }

    if (keys.enter && voiceMemoCount > 0) {
      playSelectedVoiceMemo();
    }
  } else if (currentScreen == Screen::VoiceMemoDeleteConfirm) {
    if (keys.enter) {
      deleteSelectedVoiceMemo();
      setScreen(Screen::VoiceMemoDeleteResult);
    }
  } else if (currentScreen == Screen::VoiceMemoDeleteResult) {
    if (keys.enter) {
      pendingVoiceMemoDeleteName = "";
      pendingVoiceMemoDeletePath = "";
      setScreen(Screen::VoiceMemos);
    }
  }
}

void moveMenuSelection(int direction) {
  selectedMenuIndex += direction;

  if (selectedMenuIndex < 0) {
    selectedMenuIndex = MENU_ITEM_COUNT - 1;
  } else if (selectedMenuIndex >= MENU_ITEM_COUNT) {
    selectedMenuIndex = 0;
  }

  showMainMenu();
}

void activateSelectedMenuItem() {
  setScreen(MENU_ITEMS[selectedMenuIndex].screen);
}

bool isMenuBackKey(const Keyboard_Class::KeysState& keys) {
  return keys.del;
}

void sampleBatteryStatus() {
  lastBatterySampleMs = millis();
  lastBatteryVoltageMv = M5.Power.getBatteryVoltage();
  lastBatteryLevel = M5.Power.getBatteryLevel();
  lastVbusVoltageMv = M5.Power.getVBUSVoltage();
  lastBatteryCurrentMa = M5.Power.getBatteryCurrent();
  lastChargingStatus = M5.Power.isCharging();

  updateBatteryTrend(lastBatteryVoltageMv, lastBatteryLevel);
}

void updateBatteryTrend(int voltageMv, int batteryLevel) {
  if (voltageMv <= 0) {
    return;
  }

  if (!batteryTrend.initialized) {
    batteryTrend.initialized = true;
    batteryTrend.firstVoltageMv = voltageMv;
    batteryTrend.lastVoltageMv = voltageMv;
    batteryTrend.minVoltageMv = voltageMv;
    batteryTrend.maxVoltageMv = voltageMv;
    batteryTrend.firstLevel = batteryLevel;
    batteryTrend.lastLevel = batteryLevel;
    return;
  }

  if (voltageMv > batteryTrend.lastVoltageMv + 2) {
    batteryTrend.risingSamples++;
    batteryTrend.fallingSamples = 0;
  } else if (voltageMv < batteryTrend.lastVoltageMv - 2) {
    batteryTrend.fallingSamples++;
    batteryTrend.risingSamples = 0;
  }

  batteryTrend.lastVoltageMv = voltageMv;
  batteryTrend.lastLevel = batteryLevel;
  batteryTrend.minVoltageMv = min(batteryTrend.minVoltageMv, voltageMv);
  batteryTrend.maxVoltageMv = max(batteryTrend.maxVoltageMv, voltageMv);

  const bool confirmedDropFromPeak =
      batteryTrend.fallingSamples >= CHARGING_CLEAR_SAMPLES &&
      batteryTrend.maxVoltageMv - batteryTrend.lastVoltageMv >
          CHARGING_TREND_THRESHOLD_MV;
  if (confirmedDropFromPeak) {
    batteryTrend.firstVoltageMv = voltageMv;
    batteryTrend.minVoltageMv = voltageMv;
    batteryTrend.maxVoltageMv = voltageMv;
    batteryTrend.firstLevel = batteryLevel;
    batteryTrend.risingSamples = 0;
    batteryTrend.fallingSamples = 0;
    batteryTrend.trendAboveThresholdSamples = 0;
    batteryTrend.inferredCharging = false;
    return;
  }

  const int voltageDelta = batteryTrend.lastVoltageMv - batteryTrend.minVoltageMv;
  const int levelDelta =
      (batteryTrend.firstLevel >= 0 && batteryTrend.lastLevel >= 0)
          ? batteryTrend.lastLevel - batteryTrend.firstLevel
          : 0;

  if (levelDelta > 0 || voltageDelta > CHARGING_TREND_THRESHOLD_MV) {
    batteryTrend.trendAboveThresholdSamples++;
  } else {
    batteryTrend.trendAboveThresholdSamples = 0;
    batteryTrend.inferredCharging = false;
  }

  if (batteryTrend.trendAboveThresholdSamples >= CHARGING_CONFIRM_SAMPLES) {
    batteryTrend.inferredCharging = true;
  }
}

const char* chargingStatusText(m5::Power_Class::is_charging_t status) {
  switch (status) {
    case m5::Power_Class::is_charging:
      return "Charging";
    case m5::Power_Class::is_discharging:
      return "Not charging";
    case m5::Power_Class::charge_unknown:
    default:
      return "Unknown";
  }
}

const char* inferredChargingText(m5::Power_Class::is_charging_t status) {
  if (status == m5::Power_Class::is_charging) {
    return "Charging";
  }

  if (status == m5::Power_Class::is_discharging) {
    return "Not charging";
  }

  if (!batteryTrend.initialized) {
    return "Not charging";
  }

  if (batteryTrend.inferredCharging) {
    return "Charging";
  }

  return "Not charging";
}

void printKeyState(const Keyboard_Class::KeysState& keys) {
  Serial.print("Keys:");

  for (char key : keys.word) {
    Serial.print(' ');
    Serial.print(key);
  }

  if (keys.del) {
    Serial.print(" Backspace");
  }
  if (keys.enter) {
    Serial.print(" OK");
  }

  Serial.println();
}
