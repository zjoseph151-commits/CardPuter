#include "app.h"

namespace {

String trimmedConfigValue(const String& line) {
  const int equalsIndex = line.indexOf('=');
  if (equalsIndex < 0) {
    return "";
  }

  String value = line.substring(equalsIndex + 1);
  value.trim();
  return value;
}

}  // namespace

void showWifiConnect() {
  wifiConnectStatus = "Reading config...";
  wifiConnectIp = "";
  renderWifiConnect();
  connectWifiFromConfig();
  renderWifiConnect();
}

void renderWifiConnect() {
  beginContentDraw();

  contentCanvas.printf("Status: %s\n", wifiConnectStatus.substring(0, 22).c_str());

  if (wifiConnectSsid.length() > 0) {
    contentCanvas.printf("SSID: %s\n", wifiConnectSsid.substring(0, 20).c_str());
  } else {
    contentCanvas.println("Config: /config/wifi.txt");
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnectIp = WiFi.localIP().toString();
  }

  if (wifiConnectIp.length() > 0) {
    contentCanvas.printf("IP: %s\n", wifiConnectIp.c_str());
  } else {
    contentCanvas.printf("WiFi: %s\n", wifiStatusText(WiFi.status()));
  }

  contentCanvas.println();
  contentCanvas.println("OK retry");
  contentCanvas.println("D disconnect");
  contentCanvas.println("Back menu");
  commitContentDraw();
}

bool initWifiConfigSd() {
  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

  if (!SD.begin(SD_SPI_CS_PIN, SPI, SD_SPI_FREQUENCY)) {
    wifiConnectStatus = "SD init failed.";
    Serial.println("WiFi connect: SD init failed.");
    return false;
  }

  if (SD.cardType() == CARD_NONE) {
    wifiConnectStatus = "No SD card.";
    Serial.println("WiFi connect: no SD card.");
    return false;
  }

  return true;
}

bool readWifiCredentialsFromSd(String& wifiSsid, String& wifiPassword) {
  wifiSsid = "";
  wifiPassword = "";

  if (!initWifiConfigSd()) {
    return false;
  }

  if (!SD.exists(WIFI_CONFIG_PATH)) {
    wifiConnectStatus = "No wifi config.";
    Serial.printf("WiFi connect: missing %s\n", WIFI_CONFIG_PATH);
    return false;
  }

  File configFile = SD.open(WIFI_CONFIG_PATH, FILE_READ);
  if (!configFile) {
    wifiConnectStatus = "Config open failed.";
    Serial.printf("WiFi connect: could not open %s\n", WIFI_CONFIG_PATH);
    return false;
  }

  while (configFile.available()) {
    String line = configFile.readStringUntil('\n');
    line.trim();

    if (line.length() == 0 || line.startsWith("#")) {
      continue;
    }

    const int equalsIndex = line.indexOf('=');
    if (equalsIndex < 0) {
      continue;
    }

    String key = line.substring(0, equalsIndex);
    key.trim();
    key.toLowerCase();

    if (key == "ssid") {
      wifiSsid = trimmedConfigValue(line);
    } else if (key == "password") {
      wifiPassword = trimmedConfigValue(line);
    }
  }

  configFile.close();
  wifiSsid.trim();
  wifiPassword.trim();

  if (wifiSsid.length() == 0) {
    wifiConnectStatus = "Missing SSID.";
    Serial.printf("WiFi connect: %s has no ssid value.\n", WIFI_CONFIG_PATH);
    return false;
  }

  return true;
}

bool connectWifiFromConfig() {
  String wifiPassword;

  if (!readWifiCredentialsFromSd(wifiConnectSsid, wifiPassword)) {
    wifiConnectIp = "";
    return false;
  }

  wifiConnectStatus = "Connecting...";
  wifiConnectIp = "";
  renderWifiConnect();

  Serial.printf("WiFi connect: connecting to %s\n", wifiConnectSsid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  delay(100);
  WiFi.begin(wifiConnectSsid.c_str(), wifiPassword.c_str());

  const uint32_t startedAtMs = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedAtMs < WIFI_CONNECT_TIMEOUT_MS) {
    M5Cardputer.update();
    delay(WIFI_CONNECT_POLL_MS);
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnectIp = WiFi.localIP().toString();
    wifiConnectStatus = "Connected.";
    Serial.printf("WiFi connect: connected, IP=%s\n", wifiConnectIp.c_str());
    return true;
  }

  wifiConnectStatus = String("Failed: ") + wifiStatusText(WiFi.status());
  wifiConnectIp = "";
  Serial.printf("WiFi connect: failed, status=%s\n", wifiStatusText(WiFi.status()));
  WiFi.disconnect(false, false);
  return false;
}

void disconnectWifi() {
  WiFi.disconnect(false, false);
  wifiConnectIp = "";
  wifiConnectStatus = "Disconnected.";
  Serial.println("WiFi connect: disconnected.");
  renderWifiConnect();
}

const char* wifiStatusText(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS:
      return "Idle";
    case WL_NO_SSID_AVAIL:
      return "No SSID";
    case WL_SCAN_COMPLETED:
      return "Scan done";
    case WL_CONNECTED:
      return "Connected";
    case WL_CONNECT_FAILED:
      return "Connect failed";
    case WL_CONNECTION_LOST:
      return "Lost";
    case WL_DISCONNECTED:
      return "Disconnected";
    default:
      return "Unknown";
  }
}
