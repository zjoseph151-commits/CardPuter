#include "app.h"

const uint8_t NRF24_SHARED_ADDRESS[5] = {'S', 'C', 'B', 'R', '1'};

const MenuItem MENU_ITEMS[] = {
    {"Battery", Screen::BatteryInfo},
    {"System", Screen::SystemInfo},
    {"WiFi Scan", Screen::WifiScan},
    {"Saved WiFi", Screen::SavedWifi},
    {"Voice Memos", Screen::VoiceMemos},
    {"Environment", Screen::Environment},
    {"RF Scan", Screen::RfScanner},
    {"Level", Screen::LevelTool},
};
const int MENU_ITEM_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);

SHT3X envSht30;
QMP6988 envQmp6988;
RF24 nrf24Radio(NRF24_SPI_FREQUENCY);
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
bool envLogging = false;
bool nrf24Initialized = false;
bool nrf24ChipConnected = false;
bool nrf24Listening = false;
File voiceMemoFile;
File envLogFile;
String voiceMemoStatus;
String activeVoiceMemoName;
String activeVoiceMemoPath;
String pendingVoiceMemoDeleteName;
String pendingVoiceMemoDeletePath;
String voiceMemoDeleteResultMessage;
String envStatus = "Not initialized.";
String envLogStatus;
String envLogFileName;
String envLogFilePath;
String envLogNameInput;
String nrf24Status = "Not initialized.";
String rfScanStatus = "Not scanned.";
uint32_t voiceMemoRecordedBytes = 0;
uint32_t envLogSampleCount = 0;
uint32_t rfScanCompletedAtMs = 0;
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
bool rfScanHasData = false;
bool rfScanInProgress = false;
uint8_t rfScanActivity[RF_SCAN_CHANNEL_COUNT] = {};
uint8_t rfScanQuietChannels[RF_SCAN_QUIET_COUNT] = {};
uint8_t rfScanSelectedChannel = NRF24_CHANNEL;
