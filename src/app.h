#pragma once

#include <Arduino.h>
#include <M5Cardputer.h>
#include <M5UnitENV.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <SD.h>
#include <time.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <math.h>

constexpr const char* FIRMWARE_NAME = "Scoober";
constexpr const char* FIRMWARE_VERSION = "v0.1.0";

constexpr int HEADER_HEIGHT = 22;
constexpr int CONTENT_TOP = HEADER_HEIGHT + 4;
constexpr uint8_t CONTENT_CANVAS_COLOR_DEPTH = 16;
constexpr uint8_t CONTENT_CANVAS_FALLBACK_COLOR_DEPTH = 8;
constexpr int CHARGING_TREND_THRESHOLD_MV = 50;
constexpr int CHARGING_CONFIRM_SAMPLES = 3;
constexpr int CHARGING_CLEAR_SAMPLES = 3;
constexpr int VBUS_PRESENT_THRESHOLD_MV = 4500;
constexpr int MAIN_MENU_VISIBLE_ROWS = 8;
constexpr int MAX_WIFI_NETWORKS = 40;
constexpr int WIFI_VISIBLE_ROWS = 5;
constexpr int MAX_SAVED_WIFI_NAMES = 20;
constexpr int SAVED_WIFI_VISIBLE_ROWS = 5;
constexpr const char* WIFI_CONFIG_PATH = "/config/wifi.txt";
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t WIFI_CONNECT_POLL_MS = 250;
constexpr const char* PI_CONFIG_PATH = "/config/pi.txt";
constexpr const char* PI_MONITOR_DEFAULT_DEVICE_ID = "scoober-cardputer";
constexpr uint16_t PI_MQTT_DEFAULT_PORT = 1883;
constexpr uint16_t PI_MQTT_PACKET_BUFFER_SIZE = 512;
constexpr uint16_t PI_MQTT_SOCKET_TIMEOUT_SECONDS = 2;
constexpr uint16_t PI_MQTT_KEEPALIVE_SECONDS = 30;
constexpr uint32_t PI_MONITOR_RESPONSE_WINDOW_MS = 30000;
constexpr uint32_t PI_MONITOR_STATUS_PUBLISH_INTERVAL_MS = 60000;
constexpr int MAX_PI_MONITOR_DEVICES = 6;
constexpr int PI_MONITOR_VISIBLE_DEVICES = 1;
constexpr int PI_MONITOR_SET_INTERVAL_OPTION_COUNT = 4;
constexpr uint16_t PI_MONITOR_SET_INTERVAL_OPTIONS_SECONDS[PI_MONITOR_SET_INTERVAL_OPTION_COUNT] = {
    10, 30, 60, 300};
constexpr const char* PI_MONITOR_READ_NOW_PAYLOAD = "{\"command\":\"read_now\"}";
constexpr int SD_SPI_SCK_PIN = 40;
constexpr int SD_SPI_MISO_PIN = 39;
constexpr int SD_SPI_MOSI_PIN = 14;
constexpr int SD_SPI_CS_PIN = 12;
constexpr int SD_SPI_FREQUENCY = 25000000;
constexpr int SHARED_SPI_OWNER_NONE = 0;
constexpr int SHARED_SPI_OWNER_SD = 1;
constexpr int SHARED_SPI_OWNER_LORA = 2;
constexpr int LORA_RST_PIN = 3;
constexpr int LORA_IRQ_PIN = 4;
constexpr int LORA_NSS_PIN = 5;
constexpr int LORA_BUSY_PIN = 6;
constexpr int LORA_SPI_SCK_PIN = 40;
constexpr int LORA_SPI_MISO_PIN = 39;
constexpr int LORA_SPI_MOSI_PIN = 14;
constexpr int LORA_GNSS_TX_PIN = 13;
constexpr int LORA_GNSS_RX_PIN = 15;
constexpr uint32_t LORA_GNSS_BAUD = 115200;
constexpr uint8_t LORA_IO_EXPANDER_ADDRESS = 0x43;
constexpr uint8_t LORA_RF_SWITCH_PIN = 0;
constexpr float LORA_DIAG_RX_FREQUENCY_MHZ = 915.0f;
constexpr float LORA_DIAG_BANDWIDTH_KHZ = 125.0f;
constexpr uint8_t LORA_DIAG_SPREADING_FACTOR = 12;
constexpr uint8_t LORA_DIAG_CODING_RATE = 5;
constexpr uint8_t LORA_DIAG_SYNC_WORD = 0x34;
constexpr int8_t LORA_DIAG_UNUSED_TX_POWER_DBM = 2;
constexpr uint8_t LORA_DIAG_PREAMBLE_LEN = 20;
constexpr uint32_t LORA_DIAG_SERVICE_INTERVAL_MS = 200;
constexpr uint32_t LORA_DIAG_RENDER_INTERVAL_MS = 1000;
constexpr uint32_t LORA_GNSS_FIX_STALE_MS = 5000;
constexpr size_t LORA_GNSS_MAX_LINE_CHARS = 82;
constexpr const char* LORA_DIAG_NO_TX_NOTICE = "RX only. No TX.";
constexpr uint32_t GNSS_DASH_SERVICE_INTERVAL_MS = 200;
constexpr uint32_t GNSS_DASH_RENDER_INTERVAL_MS = 1000;
constexpr int GNSS_SKY_MAX_SATELLITES = 24;
constexpr uint32_t GNSS_SKY_STALE_MS = 15000;
constexpr uint32_t GNSS_SKY_SERVICE_INTERVAL_MS = 200;
constexpr uint32_t GNSS_SKY_RENDER_INTERVAL_MS = 1000;
constexpr uint32_t RETURN_HOME_SERVICE_INTERVAL_MS = 200;
constexpr uint32_t RETURN_HOME_RENDER_INTERVAL_MS = 1000;
constexpr float RETURN_HOME_ARRIVAL_RADIUS_METERS = 10.0f;
constexpr const char* RETURN_HOME_PREF_NAMESPACE = "scoober_home";
constexpr int ENV_I2C_SDA_PIN = 2;
constexpr int ENV_I2C_SCL_PIN = 1;
constexpr uint32_t ENV_I2C_FREQUENCY = 400000U;
constexpr uint8_t I2C_HUB_ADDRESS = 0x70;
constexpr uint8_t I2C_HUB_CHANNEL_COUNT = 6;
constexpr uint8_t I2C_HUB_ENV_CHANNEL = 0;
constexpr uint8_t I2C_HUB_OLED_CHANNEL = 1;
constexpr uint8_t I2C_HUB_RTC_CHANNEL = 5;
constexpr uint16_t I2C_HUB_CHANNEL_SETTLE_US = 1000;
constexpr uint8_t OLED_I2C_ADDRESS_PRIMARY = 0x3C;
constexpr uint8_t OLED_I2C_ADDRESS_SECONDARY = 0x3D;
constexpr uint32_t OLED_TEST_REFRESH_INTERVAL_MS = 1000;
constexpr uint32_t OLED_STATUS_REFRESH_INTERVAL_MS = 1000;
constexpr uint32_t OLED_RETRY_INTERVAL_MS = 3000;
constexpr uint8_t OLED_STATUS_LINE_COUNT = 5;
constexpr uint8_t OLED_STATUS_MAX_CHARS = 21;
constexpr uint8_t RTC_DS3231_ADDRESS = 0x68;
constexpr uint8_t RTC_AT24C32_ADDRESS = 0x57;
constexpr uint8_t RTC_DS3231_SECONDS_REGISTER = 0x00;
constexpr uint8_t RTC_DS3231_CONTROL_REGISTER = 0x0E;
constexpr uint8_t RTC_DS3231_STATUS_REGISTER = 0x0F;
constexpr uint8_t RTC_DS3231_TEMPERATURE_REGISTER = 0x11;
constexpr uint32_t RTC_REFRESH_INTERVAL_MS = 1000;
constexpr uint32_t RTC_RETRY_INTERVAL_MS = 3000;
constexpr uint32_t RTC_NTP_SYNC_TIMEOUT_MS = 10000;
constexpr uint32_t RTC_NTP_POLL_MS = 250;
constexpr const char* RTC_TIMEZONE_POSIX = "MST7MDT,M3.2.0,M11.1.0";
constexpr const char* RTC_NTP_SERVER_PRIMARY = "pool.ntp.org";
constexpr const char* RTC_NTP_SERVER_SECONDARY = "time.nist.gov";
constexpr float ENV_PRESSURE_MIN_HPA = 300.0f;
constexpr float ENV_PRESSURE_MAX_HPA = 1100.0f;
constexpr uint32_t ENV_REFRESH_INTERVAL_MS = 1000;
constexpr uint32_t ENV_RETRY_INTERVAL_MS = 3000;
constexpr const char* ENV_LOG_DIR = "/env";
constexpr const char* ENV_LOG_HEADER =
    "uptime_s,temp_c,temp_f,humidity_pct,pressure_hpa";
constexpr int ENV_LOG_NAME_MAX_LENGTH = 16;
constexpr const char* VOICE_MEMO_DIR = "/memos";
constexpr int MAX_VOICE_MEMOS = 30;
constexpr int VOICE_MEMO_VISIBLE_ROWS = 5;
constexpr int VOICE_MEMO_ADV_MIC_DATA_PIN = 46;
constexpr int VOICE_MEMO_ADV_MIC_WS_PIN = 43;
constexpr int VOICE_MEMO_ADV_MIC_BCK_PIN = 41;
constexpr uint8_t VOICE_MEMO_ES8311_ADDRESS = 0x18;
constexpr uint32_t VOICE_MEMO_AUDIO_I2C_FREQUENCY = 100000;
constexpr size_t VOICE_RECORD_SAMPLE_RATE = 16000;
constexpr size_t VOICE_RECORD_CHUNK_SAMPLES = 240;
constexpr uint32_t VOICE_RECORD_CHUNK_READY_MS =
    static_cast<uint32_t>(((VOICE_RECORD_CHUNK_SAMPLES * 1000UL) +
                           VOICE_RECORD_SAMPLE_RATE - 1) /
                          VOICE_RECORD_SAMPLE_RATE) +
    4;
constexpr uint32_t VOICE_RECORD_CHUNK_TIMEOUT_MS =
    VOICE_RECORD_CHUNK_READY_MS + 250;
constexpr uint8_t VOICE_MEMO_MIC_PROBE_CHUNKS = 3;
constexpr int16_t VOICE_RECORD_SILENT_PEAK_THRESHOLD = 12;
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
  PiMonitor,
  VoiceMemos,
  VoiceMemoDeleteConfirm,
  VoiceMemoDeleteResult,
  Environment,
  EnvironmentLogName,
  OledTest,
  RtcStatus,
  GnssDashboard,
  GnssSkyView,
  ReturnHome,
  LoraDiag,
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

struct PiMonitorDevice {
  bool active = false;
  String id;
  String availability;
  String summary;
  String topicKind;
  unsigned long lastSeenMs = 0;
};

struct GnssSkySatellite {
  bool active = false;
  char constellation = '?';
  uint16_t prn = 0;
  int16_t elevationDeg = 0;
  uint16_t azimuthDeg = 0;
  int16_t snrDb = -1;
  unsigned long lastSeenMs = 0;
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

extern const MenuItem MENU_ITEMS[];
extern const int MENU_ITEM_COUNT;

extern SHT3X envSht30;
extern QMP6988 envQmp6988;
extern U8G2_SSD1309_128X64_NONAME0_F_HW_I2C oledDisplay;
extern WiFiClient piMonitorWifiClient;
extern PubSubClient piMonitorMqttClient;
extern Screen currentScreen;
extern int selectedMenuIndex;
extern int menuScrollOffset;
extern unsigned long lastSystemRefreshMs;
extern unsigned long lastLevelRefreshMs;
extern unsigned long lastOledRefreshMs;
extern unsigned long lastOledInitAttemptMs;
extern unsigned long lastRtcRefreshMs;
extern unsigned long lastRtcRetryMs;
extern unsigned long lastBatterySampleMs;
extern unsigned long lastEnvironmentRefreshMs;
extern unsigned long lastEnvironmentRetryMs;
extern unsigned long lastLoraDiagServiceMs;
extern unsigned long lastLoraDiagRenderMs;
extern unsigned long lastGnssDashboardServiceMs;
extern unsigned long lastGnssDashboardRenderMs;
extern unsigned long lastGnssSkyViewServiceMs;
extern unsigned long lastGnssSkyViewRenderMs;
extern unsigned long lastGnssSkyGsvMs;
extern unsigned long lastReturnHomeServiceMs;
extern unsigned long lastReturnHomeRenderMs;
extern M5Canvas contentCanvas;
extern bool contentCanvasReady;
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
extern String piMonitorStatus;
extern String piMonitorBrokerHost;
extern uint16_t piMonitorBrokerPort;
extern String piMonitorDeviceId;
extern String piMonitorCommandTarget;
extern String piMonitorSelectedCommandTarget;
extern String piMonitorCommandStatus;
extern uint32_t piMonitorCommandCount;
extern int piMonitorSetIntervalIndex;
extern uint32_t piMonitorSetIntervalCommandCount;
extern String piMonitorLastResponseDevice;
extern String piMonitorLastResponseSummary;
extern uint32_t piMonitorResponseCount;
extern unsigned long piMonitorLastCommandSentMs;
extern uint32_t piMonitorStatusPublishCount;
extern unsigned long piMonitorLastStatusPublishMs;
extern bool piMonitorConfigLoaded;
extern String piMonitorLastTopic;
extern String piMonitorLastPayload;
extern uint32_t piMonitorMessageCount;
extern PiMonitorDevice piMonitorDevices[MAX_PI_MONITOR_DEVICES];
extern VoiceMemoFile voiceMemos[MAX_VOICE_MEMOS];
extern int voiceMemoCount;
extern int selectedVoiceMemoIndex;
extern int voiceMemoScrollOffset;
extern bool voiceSdInitialized;
extern bool voiceSdAvailable;
extern bool voiceMemoRecording;
extern bool voiceMemoPlaying;
extern bool voiceMemoChunkPending;
extern bool envSensorInitialized;
extern bool envSht30Ready;
extern bool envQmp6988Ready;
extern bool envHasTempHumidity;
extern bool envHasPressure;
extern bool envPressureInvalid;
extern bool i2cHubDetected;
extern bool oledInitialized;
extern bool oledOnline;
extern bool rtcInitialized;
extern bool rtcOnline;
extern bool rtcEepromDetected;
extern bool rtcTimeValid;
extern bool rtcOscillatorStopped;
extern bool rtcTemperatureValid;
extern bool loraDiagInitialized;
extern bool loraIoExpanderDetected;
extern bool loraRfSwitchEnabled;
extern bool loraRadioReady;
extern bool loraListening;
extern bool loraGnssStarted;
extern bool loraGnssLocationValid;
extern bool loraGnssSatellitesValid;
extern bool loraGnssHdopValid;
extern bool loraGnssTimeValid;
extern bool loraGnssDateValid;
extern bool loraGnssSpeedValid;
extern bool loraGnssAltitudeValid;
extern bool gnssDashboardInitialized;
extern bool gnssSkyViewInitialized;
extern bool returnHomeInitialized;
extern bool returnHomeWaypointValid;
extern bool returnHomeNavigationValid;
extern bool envLogging;
extern File voiceMemoFile;
extern File envLogFile;
extern String voiceMemoStatus;
extern String activeVoiceMemoName;
extern String activeVoiceMemoPath;
extern String voiceMemoMicStatus;
extern String pendingVoiceMemoDeleteName;
extern String pendingVoiceMemoDeletePath;
extern String voiceMemoDeleteResultMessage;
extern String envStatus;
extern String i2cHubStatus;
extern String oledStatus;
extern String oledStatusLine;
extern String rtcStatus;
extern String loraStatus;
extern String loraRadioStatus;
extern String loraGnssStatus;
extern String returnHomeStatus;
extern String loraLastPacket;
extern String loraLastNmeaLine;
extern String envLogStatus;
extern String envLogFileName;
extern String envLogFilePath;
extern String envLogNameInput;
extern uint32_t voiceMemoRecordedBytes;
extern uint32_t voiceMemoSilentChunkCount;
extern uint32_t envLogSampleCount;
extern uint32_t oledDrawCount;
extern unsigned long voiceMemoRecordingStartedMs;
extern unsigned long lastVoiceMemoRenderMs;
extern unsigned long voiceMemoChunkStartedMs;
extern int16_t voiceRecordBuffer[VOICE_RECORD_CHUNK_SAMPLES];
extern int16_t voiceMemoLastPeak;
extern int lastBatteryVoltageMv;
extern int lastBatteryLevel;
extern int lastVbusVoltageMv;
extern int lastBatteryCurrentMa;
extern int i2cHubActiveChannel;
extern int loraRadioState;
extern int sharedSpiOwner;
extern int selectedGnssSkySatelliteIndex;
extern uint8_t oledActiveAddress;
extern uint8_t rtcMonth;
extern uint8_t rtcDay;
extern uint8_t rtcHour;
extern uint8_t rtcMinute;
extern uint8_t rtcSecond;
extern uint8_t rtcDayOfWeek;
extern m5::Power_Class::is_charging_t lastChargingStatus;
extern GnssSkySatellite gnssSkySatellites[GNSS_SKY_MAX_SATELLITES];
extern float envTemperatureC;
extern float envHumidityPercent;
extern float envPressureHpa;
extern float rtcTemperatureC;
extern float loraLastRssi;
extern float loraLastSnr;
extern double loraGnssLatitude;
extern double loraGnssLongitude;
extern double returnHomeLatitude;
extern double returnHomeLongitude;
extern float loraGnssHdop;
extern float loraGnssSpeedKmph;
extern float loraGnssAltitudeMeters;
extern float returnHomeDistanceMeters;
extern float returnHomeBearingDeg;
extern uint32_t loraPacketCount;
extern uint32_t loraCrcErrorCount;
extern uint32_t loraReceiveErrorCount;
extern uint32_t loraGnssByteCount;
extern uint32_t loraGnssLineCount;
extern uint32_t loraGnssCharsParsed;
extern uint32_t loraGnssPassedChecksum;
extern uint32_t loraGnssFailedChecksum;
extern uint32_t loraGnssFixAgeMs;
extern uint32_t loraGnssSatellites;
extern uint32_t rtcReadAttemptCount;
extern uint32_t gnssSkySatelliteCount;
extern uint32_t gnssSkySatellitesInView;
extern uint32_t gnssSkyGsvSentenceCount;
extern uint16_t loraGnssYear;
extern uint16_t rtcYear;
extern uint8_t loraGnssMonth;
extern uint8_t loraGnssDay;
extern uint8_t loraGnssHour;
extern uint8_t loraGnssMinute;
extern uint8_t loraGnssSecond;
extern float smoothedLevelX;
extern float smoothedLevelY;
extern bool levelSmoothingInitialized;

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
void showPiMonitor();
void renderPiMonitor();
void showVoiceMemos();
void renderVoiceMemos();
void renderVoiceMemoDeleteConfirm();
void renderVoiceMemoDeleteResult();
void showEnvironment();
void renderEnvironmentLogName();
void showOledTest();
void renderOledTest();
void serviceOledStatusDashboard();
void renderOledStatusDashboard();
void setOledStatusLine(const String& line);
void clearOledStatusLine();
void showRtcStatus();
void renderRtcStatus();
bool initRtcStatus();
void serviceRtcStatus();
void resetRtcStatus();
bool readRtcStatus();
bool setRtcToBuildTime();
bool setRtcFromNtp();
String rtcDateText();
String rtcTimeText();
String rtcTemperatureText();
void showLoraDiag();
void renderLoraDiag();
bool initLoraDiagnostics();
void serviceLoraDiagnostics();
void resetLoraDiagnostics();
void stopLoraDiagnostics();
void showGnssDashboard();
void renderGnssDashboard();
bool initGnssDashboard();
void serviceGnssDashboard();
void resetGnssDashboard();
void stopGnssDashboard();
void showGnssSkyView();
void renderGnssSkyView();
bool initGnssSkyView();
void serviceGnssSkyView();
void resetGnssSkyView();
void stopGnssSkyView();
void showReturnHome();
void renderReturnHome();
bool initReturnHome();
void serviceReturnHome();
void resetReturnHome();
void stopReturnHome();
bool loadReturnHomeWaypoint();
bool saveReturnHomeWaypoint();
bool clearReturnHomeWaypoint();
void updateReturnHomeNavigation();
String returnHomeDistanceText();
String returnHomeBearingText();
bool loraGnssHasFreshFix();
String loraGnssSatellitesText();
String loraGnssHdopText();
String loraGnssCoordinateText(const char* label, double value);
String loraGnssTimeText();
String loraGnssUtcText();
String loraGnssDateText();
String loraGnssSpeedText();
String loraGnssAltitudeText();
void startLoraGnssSerial();
void serviceLoraGnssSerial();
void resetLoraGnssParser();
void stopLoraGnssSerial();
void refreshGnssSkySatellites();
void clampGnssSkySelection();
void moveGnssSkySelection(int direction);
const GnssSkySatellite* selectedGnssSkySatellite();
String gnssSkySatelliteLabel(const GnssSkySatellite& satellite);
String gnssSkyConstellationName(char constellation);
String gnssSkySatelliteSnrText(const GnssSkySatellite& satellite);
String gnssSkySatelliteAgeText(const GnssSkySatellite& satellite);
String gnssSkyCompassDirection(uint16_t azimuthDeg);
void showLevelTool();

void setScreen(Screen screen);
void drawHeader(const char* title);
void drawScreenFrame(const char* title);
bool initContentCanvas();
void beginContentDraw();
void commitContentDraw();
void deselectSharedSpiDevices();
void prepareSharedSpiForSd();
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
bool initPiMonitorConfigSd();
bool readPiMonitorConfigFromSd();
bool connectPiMonitorMqtt();
void disconnectPiMonitorMqtt();
uint16_t selectedPiMonitorSetIntervalSeconds();
void cyclePiMonitorCommandTarget();
void cyclePiMonitorSetInterval();
bool publishPiMonitorReadNowCommand();
bool publishPiMonitorSetIntervalCommand();
void stopPiMonitor();
void servicePiMonitor();
void clearPiMonitorDevices();
void handlePiMonitorMessage(char* topic, byte* payload, unsigned int length);
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
bool isValidEnvironmentPressureHpa(float pressureHpa);
bool detectI2cHub();
bool selectI2cHubChannel(uint8_t channel);
bool selectEnvironmentI2cPath();
bool selectOledI2cPath();
bool selectRtcI2cPath();
String environmentI2cPathLabel();
String oledI2cPathLabel();
String rtcI2cPathLabel();
bool initOledDisplay();
bool ensureOledReady();
bool probeOledAddress(uint8_t address);
void drawOledTestPattern();
bool initEnvironmentLogSd();
String sanitizeEnvironmentLogName(const String& requestedName);
bool findNextEnvironmentLogPath(const String& requestedName, String& path, String& name);
bool startEnvironmentLogging();
bool startEnvironmentLogging(const String& requestedName);
void stopEnvironmentLogging(const char* message);
void appendEnvironmentLogSample();
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
