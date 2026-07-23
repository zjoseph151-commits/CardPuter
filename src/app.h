#pragma once

#include <Arduino.h>
#include <M5Cardputer.h>
#include <M5UnitENV.h>
#include <Preferences.h>
#include <RF24.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <WiFi.h>
#include <math.h>

constexpr const char* FIRMWARE_NAME = "Scoober";
constexpr const char* FIRMWARE_VERSION = "v0.1.0";

constexpr int HEADER_HEIGHT = 22;
constexpr int CONTENT_TOP = HEADER_HEIGHT + 4;
constexpr int CHARGING_TREND_THRESHOLD_MV = 50;
constexpr int CHARGING_CONFIRM_SAMPLES = 3;
constexpr int CHARGING_CLEAR_SAMPLES = 3;
constexpr int VBUS_PRESENT_THRESHOLD_MV = 4500;
constexpr int MAX_WIFI_NETWORKS = 40;
constexpr int WIFI_VISIBLE_ROWS = 5;
constexpr int MAX_SAVED_WIFI_NAMES = 20;
constexpr int SAVED_WIFI_VISIBLE_ROWS = 5;
constexpr const char* WIFI_CONFIG_PATH = "/config/wifi.txt";
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t WIFI_CONNECT_POLL_MS = 250;
constexpr int SD_SPI_SCK_PIN = 40;
constexpr int SD_SPI_MISO_PIN = 39;
constexpr int SD_SPI_MOSI_PIN = 14;
constexpr int SD_SPI_CS_PIN = 12;
constexpr int SD_SPI_FREQUENCY = 25000000;
constexpr int NRF24_SPI_SCK_PIN = 40;
constexpr int NRF24_SPI_MISO_PIN = 39;
constexpr int NRF24_SPI_MOSI_PIN = 14;
constexpr int NRF24_CSN_PIN = 5;
constexpr int NRF24_CE_PIN = 4;
constexpr uint32_t NRF24_SPI_FREQUENCY = 4000000;
constexpr uint8_t NRF24_CHANNEL = 76;
constexpr uint8_t NRF24_PAYLOAD_SIZE = 32;
constexpr uint8_t NRF24_RX_PIPE = 1;
constexpr rf24_datarate_e NRF24_DATA_RATE = RF24_250KBPS;
constexpr uint8_t RF_SCAN_CHANNEL_COUNT = 126;
constexpr uint8_t RF_SCAN_SAMPLE_COUNT = 5;
constexpr uint8_t RF_SCAN_QUIET_COUNT = 5;
constexpr uint8_t RF_SCAN_MIN_QUIET_SPACING = 5;
constexpr uint16_t RF_SCAN_DWELL_MS = 2;
constexpr int ENV_I2C_SDA_PIN = 2;
constexpr int ENV_I2C_SCL_PIN = 1;
constexpr uint32_t ENV_I2C_FREQUENCY = 400000U;
constexpr uint32_t ENV_REFRESH_INTERVAL_MS = 1000;
constexpr uint32_t ENV_RETRY_INTERVAL_MS = 3000;
constexpr const char* ENV_LOG_DIR = "/env";
constexpr const char* ENV_LOG_HEADER =
    "uptime_s,temp_c,temp_f,humidity_pct,pressure_hpa,altitude_m";
constexpr int ENV_LOG_NAME_MAX_LENGTH = 16;
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
  WifiConnect,
  VoiceMemos,
  VoiceMemoDeleteConfirm,
  VoiceMemoDeleteResult,
  Environment,
  EnvironmentLogName,
  RfScanner,
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

extern const uint8_t NRF24_SHARED_ADDRESS[5];
extern const MenuItem MENU_ITEMS[];
extern const int MENU_ITEM_COUNT;

extern SHT3X envSht30;
extern QMP6988 envQmp6988;
extern RF24 nrf24Radio;
extern Screen currentScreen;
extern int selectedMenuIndex;
extern unsigned long lastSystemRefreshMs;
extern unsigned long lastLevelRefreshMs;
extern unsigned long lastBatterySampleMs;
extern unsigned long lastEnvironmentRefreshMs;
extern unsigned long lastEnvironmentRetryMs;
extern M5Canvas contentCanvas;
extern BatteryTrend batteryTrend;
extern WifiNetwork wifiNetworks[MAX_WIFI_NETWORKS];
extern int wifiNetworkCount;
extern int selectedWifiIndex;
extern int wifiScrollOffset;
extern Preferences wifiPrefs;
extern String savedWifiNames[MAX_SAVED_WIFI_NAMES];
extern int savedWifiCount;
extern int selectedSavedWifiIndex;
extern int savedWifiScrollOffset;
extern String wifiSaveMessage;
extern String wifiSaveResultSsid;
extern String pendingDeleteWifiName;
extern int pendingDeleteWifiIndex;
extern String savedWifiDeleteResultName;
extern String savedWifiDeleteResultMessage;
extern String wifiConnectStatus;
extern String wifiConnectSsid;
extern String wifiConnectIp;
extern VoiceMemoFile voiceMemos[MAX_VOICE_MEMOS];
extern int voiceMemoCount;
extern int selectedVoiceMemoIndex;
extern int voiceMemoScrollOffset;
extern bool voiceSdInitialized;
extern bool voiceSdAvailable;
extern bool voiceMemoRecording;
extern bool voiceMemoPlaying;
extern bool envSensorInitialized;
extern bool envSht30Ready;
extern bool envQmp6988Ready;
extern bool envHasTempHumidity;
extern bool envHasPressure;
extern bool envLogging;
extern bool nrf24Initialized;
extern bool nrf24ChipConnected;
extern bool nrf24Listening;
extern File voiceMemoFile;
extern File envLogFile;
extern String voiceMemoStatus;
extern String activeVoiceMemoName;
extern String activeVoiceMemoPath;
extern String pendingVoiceMemoDeleteName;
extern String pendingVoiceMemoDeletePath;
extern String voiceMemoDeleteResultMessage;
extern String envStatus;
extern String envLogStatus;
extern String envLogFileName;
extern String envLogFilePath;
extern String envLogNameInput;
extern String nrf24Status;
extern String rfScanStatus;
extern uint32_t voiceMemoRecordedBytes;
extern uint32_t envLogSampleCount;
extern uint32_t rfScanCompletedAtMs;
extern unsigned long voiceMemoRecordingStartedMs;
extern unsigned long lastVoiceMemoRenderMs;
extern int16_t voiceRecordBuffer[VOICE_RECORD_CHUNK_SAMPLES];
extern int lastBatteryVoltageMv;
extern int lastBatteryLevel;
extern int lastVbusVoltageMv;
extern int lastBatteryCurrentMa;
extern m5::Power_Class::is_charging_t lastChargingStatus;
extern float envTemperatureC;
extern float envHumidityPercent;
extern float envPressureHpa;
extern float envAltitudeM;
extern float smoothedLevelX;
extern float smoothedLevelY;
extern bool levelSmoothingInitialized;
extern bool rfScanHasData;
extern bool rfScanInProgress;
extern uint8_t rfScanActivity[RF_SCAN_CHANNEL_COUNT];
extern uint8_t rfScanQuietChannels[RF_SCAN_QUIET_COUNT];
extern uint8_t rfScanSelectedChannel;

void showMainMenu();
void showBatteryInfo();
void showSystemInfo();
void showWifiScan();
void renderWifiSaveConfirm();
void renderWifiSaveResult();
void renderSavedWifiList();
void renderSavedWifiDeleteConfirm();
void renderSavedWifiDeleteResult();
void showWifiConnect();
void renderWifiConnect();
void showVoiceMemos();
void renderVoiceMemos();
void renderVoiceMemoDeleteConfirm();
void renderVoiceMemoDeleteResult();
void showEnvironment();
void renderEnvironmentLogName();
void showRfScanner();
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
bool initWifiConfigSd();
bool readWifiCredentialsFromSd(String& wifiSsid, String& wifiPassword);
bool connectWifiFromConfig();
void disconnectWifi();
const char* wifiStatusText(wl_status_t status);
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
bool initEnvironmentLogSd();
String sanitizeEnvironmentLogName(const String& requestedName);
bool findNextEnvironmentLogPath(const String& requestedName, String& path, String& name);
bool startEnvironmentLogging();
bool startEnvironmentLogging(const String& requestedName);
void stopEnvironmentLogging(const char* message);
void appendEnvironmentLogSample();
bool initNrf24Radio();
void powerDownNrf24Radio();
void resumeNrf24Listening();
bool scanRfChannels();
void renderRfScanner();
void drawRfScanGraph();
void updateQuietRfChannels();
bool isQuietRfChannel(uint8_t channel);
void moveRfScanSelection(int direction);
const char* nrf24DataRateText(rf24_datarate_e dataRate);
void resetLevelSmoothing();
void drawLevelCrosshair(int centerX, int centerY);
void drawLevelDot(int dotX, int dotY, bool isLevel);
void printKeyState(const Keyboard_Class::KeysState& keys);
bool isMenuBackKey(const Keyboard_Class::KeysState& keys);
void sampleBatteryStatus();
void updateBatteryTrend(int voltageMv, int batteryLevel);
bool isExternalPowerPresent();
bool isFilteredCharging();
bool isBatteryLevelDisplayable();
const char* chargingStatusText(m5::Power_Class::is_charging_t status);
const char* inferredChargingText(m5::Power_Class::is_charging_t status);
