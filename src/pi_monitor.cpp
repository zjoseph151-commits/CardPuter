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

const char* mqttStateText(int state) {
  switch (state) {
    case MQTT_CONNECTION_TIMEOUT:
      return "timeout";
    case MQTT_CONNECTION_LOST:
      return "lost";
    case MQTT_CONNECT_FAILED:
      return "connect failed";
    case MQTT_DISCONNECTED:
      return "disconnected";
    case MQTT_CONNECTED:
      return "connected";
    case MQTT_CONNECT_BAD_PROTOCOL:
      return "bad protocol";
    case MQTT_CONNECT_BAD_CLIENT_ID:
      return "bad client id";
    case MQTT_CONNECT_UNAVAILABLE:
      return "unavailable";
    case MQTT_CONNECT_BAD_CREDENTIALS:
      return "bad credentials";
    case MQTT_CONNECT_UNAUTHORIZED:
      return "unauthorized";
    default:
      return "unknown";
  }
}

String buildPiMonitorClientId() {
  String base = piMonitorDeviceId;
  base.trim();

  if (base.length() == 0) {
    base = PI_MONITOR_DEFAULT_DEVICE_ID;
  }

  if (base.length() > 18) {
    base = base.substring(0, 18);
  }

  char chipText[9];
  snprintf(chipText, sizeof(chipText), "%08X", static_cast<uint32_t>(ESP.getEfuseMac()));
  return base + "-" + chipText;
}

bool parsePiMonitorTopic(const char* topic, String& deviceId, String& topicKind) {
  const String topicText(topic);
  const String prefix = "home/devices/";

  if (!topicText.startsWith(prefix)) {
    return false;
  }

  const int deviceStart = prefix.length();
  const int kindSeparator = topicText.indexOf('/', deviceStart);
  if (kindSeparator < 0) {
    return false;
  }

  deviceId = topicText.substring(deviceStart, kindSeparator);
  topicKind = topicText.substring(kindSeparator + 1);
  deviceId.trim();
  topicKind.trim();
  return deviceId.length() > 0 && topicKind.length() > 0;
}

int findPiMonitorDeviceSlot(const String& deviceId) {
  int emptySlot = -1;
  int oldestSlot = 0;

  for (int i = 0; i < MAX_PI_MONITOR_DEVICES; ++i) {
    if (piMonitorDevices[i].active && piMonitorDevices[i].id == deviceId) {
      return i;
    }

    if (!piMonitorDevices[i].active && emptySlot < 0) {
      emptySlot = i;
    }

    if (piMonitorDevices[i].lastSeenMs < piMonitorDevices[oldestSlot].lastSeenMs) {
      oldestSlot = i;
    }
  }

  return emptySlot >= 0 ? emptySlot : oldestSlot;
}

int countPiMonitorDevices() {
  int count = 0;

  for (int i = 0; i < MAX_PI_MONITOR_DEVICES; ++i) {
    if (piMonitorDevices[i].active) {
      ++count;
    }
  }

  return count;
}

String summarizeStatusPayload(const StaticJsonDocument<384>& document) {
  if (document["wifi_rssi"].is<long>()) {
    char summary[24];
    const long rssi = document["wifi_rssi"].as<long>();
    snprintf(summary, sizeof(summary), "rssi:%ld", rssi);
    return summary;
  }

  const char* firmware = document["firmware_version"] | "";
  if (firmware[0] != '\0') {
    return String("fw:") + firmware;
  }

  return "status";
}

String summarizeTelemetryPayload(const StaticJsonDocument<384>& document) {
  if (document["sensor_ok"].is<bool>() && !document["sensor_ok"].as<bool>()) {
    return "sensor fail";
  }

  if (document["temperature_c"].is<float>() &&
      document["humidity_percent"].is<float>()) {
    char summary[28];
    const float temperatureC = document["temperature_c"].as<float>();
    const float humidityPercent = document["humidity_percent"].as<float>();
    snprintf(summary, sizeof(summary), "%.1fC %.0f%%", temperatureC, humidityPercent);
    return summary;
  }

  return "telemetry";
}

bool isPiMonitorResponseKind(const String& topicKind) {
  return topicKind == "responses" || topicKind == "response";
}

String summarizeResponsePayload(const String& payloadText) {
  if (payloadText.length() == 0) {
    return "empty response";
  }

  StaticJsonDocument<384> document;
  const DeserializationError error = deserializeJson(document, payloadText);
  if (error) {
    return payloadText.substring(0, 32);
  }

  const char* command = document["command"] | "";
  if (command[0] == '\0') {
    command = document["cmd"] | "";
  }

  const char* status = document["status"] | "";
  const char* errorText = document["error"] | "";
  const char* message = document["message"] | "";
  if (message[0] == '\0') {
    message = document["msg"] | "";
  }

  String summary;
  if (document["ok"].is<bool>()) {
    summary = document["ok"].as<bool>() ? "OK" : "ERR";
  } else if (document["success"].is<bool>()) {
    summary = document["success"].as<bool>() ? "OK" : "ERR";
  } else if (status[0] != '\0') {
    summary = status;
  } else {
    summary = "response";
  }

  if (command[0] != '\0') {
    summary += " ";
    summary += command;
  }

  if (errorText[0] != '\0') {
    summary += " ";
    summary += errorText;
  } else if (message[0] != '\0') {
    summary += " ";
    summary += message;
  }

  return summary.substring(0, 32);
}

String summarizeJsonPayload(const String& topicKind, const String& payloadText) {
  StaticJsonDocument<384> document;
  const DeserializationError error = deserializeJson(document, payloadText);
  if (error) {
    return topicKind + " raw";
  }

  if (topicKind == "status") {
    return summarizeStatusPayload(document);
  }

  if (topicKind == "telemetry") {
    return summarizeTelemetryPayload(document);
  }

  if (isPiMonitorResponseKind(topicKind)) {
    return summarizeResponsePayload(payloadText);
  }

  return topicKind;
}

String buildDeviceLine(const PiMonitorDevice& device) {
  String line = device.id.substring(0, 11);

  if (device.availability.length() > 0) {
    line += " ";
    line += device.availability.substring(0, 7);
  } else {
    line += " seen";
  }

  if (device.summary.length() > 0) {
    line += " ";
    line += device.summary.substring(0, 11);
  }

  return line.substring(0, 29);
}

String compactTopic(const String& topic) {
  if (topic.startsWith("home/devices/")) {
    return topic.substring(13);
  }

  if (topic.startsWith("home/")) {
    return topic.substring(5);
  }

  return topic;
}

}  // namespace

void showPiMonitor() {
  piMonitorStatus = "Reading config...";
  renderPiMonitor();
  connectPiMonitorMqtt();
  renderPiMonitor();
}

void renderPiMonitor() {
  beginContentDraw();

  const bool wifiConnected = WiFi.status() == WL_CONNECTED;
  String statusLine = piMonitorMqttClient.connected() ? "MQTT connected." : piMonitorStatus;
  contentCanvas.printf("Status: %s\n", statusLine.substring(0, 21).c_str());

  if (piMonitorBrokerHost.length() > 0) {
    String brokerLine = piMonitorBrokerHost + ":" + String(piMonitorBrokerPort);
    contentCanvas.printf("Pi: %s\n", brokerLine.substring(0, 24).c_str());
  } else {
    contentCanvas.println("Config: /config/pi.txt");
  }

  contentCanvas.printf("Msgs: %lu Devs:%d\n",
                       static_cast<unsigned long>(piMonitorMessageCount),
                       countPiMonitorDevices());
  contentCanvas.printf("Cmd: %s\n", piMonitorCommandStatus.substring(0, 22).c_str());
  if (piMonitorResponseCount > 0) {
    String responseLine = piMonitorLastResponseDevice + " " + piMonitorLastResponseSummary;
    contentCanvas.printf("Resp: %s\n", responseLine.substring(0, 22).c_str());
  } else {
    contentCanvas.println("Resp: waiting.");
  }

  if (piMonitorLastTopic.length() > 0) {
    contentCanvas.printf("Last: %s\n",
                         compactTopic(piMonitorLastTopic).substring(0, 20).c_str());
    contentCanvas.printf("Pay: %s\n", piMonitorLastPayload.substring(0, 22).c_str());
  } else {
    contentCanvas.println(wifiConnected ? "Waiting for MQTT data." : "WiFi: use WiFi Connect");
  }

  const int deviceCount = countPiMonitorDevices();

  int rendered = 0;
  for (int i = 0; i < MAX_PI_MONITOR_DEVICES && rendered < PI_MONITOR_VISIBLE_DEVICES; ++i) {
    if (!piMonitorDevices[i].active) {
      continue;
    }

    contentCanvas.println(buildDeviceLine(piMonitorDevices[i]));
    ++rendered;
  }

  if (deviceCount > PI_MONITOR_VISIBLE_DEVICES) {
    contentCanvas.printf("+%d more devices\n", deviceCount - PI_MONITOR_VISIBLE_DEVICES);
  }

  commitContentDraw();
}

bool initPiMonitorConfigSd() {
  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

  if (!SD.begin(SD_SPI_CS_PIN, SPI, SD_SPI_FREQUENCY)) {
    piMonitorStatus = "SD init failed.";
    Serial.println("Pi Monitor: SD init failed.");
    return false;
  }

  if (SD.cardType() == CARD_NONE) {
    piMonitorStatus = "No SD card.";
    Serial.println("Pi Monitor: no SD card.");
    return false;
  }

  return true;
}

bool readPiMonitorConfigFromSd() {
  piMonitorConfigLoaded = false;
  piMonitorBrokerHost = "";
  piMonitorBrokerPort = PI_MQTT_DEFAULT_PORT;
  piMonitorDeviceId = PI_MONITOR_DEFAULT_DEVICE_ID;
  piMonitorCommandTarget = "";

  if (!initPiMonitorConfigSd()) {
    return false;
  }

  if (!SD.exists(PI_CONFIG_PATH)) {
    piMonitorStatus = "No pi config.";
    Serial.printf("Pi Monitor: missing %s\n", PI_CONFIG_PATH);
    return false;
  }

  File configFile = SD.open(PI_CONFIG_PATH, FILE_READ);
  if (!configFile) {
    piMonitorStatus = "Config open failed.";
    Serial.printf("Pi Monitor: could not open %s\n", PI_CONFIG_PATH);
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

    if (key == "mqtt_host") {
      piMonitorBrokerHost = trimmedConfigValue(line);
    } else if (key == "mqtt_port") {
      const long parsedPort = trimmedConfigValue(line).toInt();
      if (parsedPort <= 0 || parsedPort > 65535) {
        piMonitorStatus = "Bad mqtt_port.";
        Serial.println("Pi Monitor: invalid mqtt_port.");
        configFile.close();
        return false;
      }
      piMonitorBrokerPort = static_cast<uint16_t>(parsedPort);
    } else if (key == "device_id") {
      piMonitorDeviceId = trimmedConfigValue(line);
    } else if (key == "command_target") {
      piMonitorCommandTarget = trimmedConfigValue(line);
    }
  }

  configFile.close();
  piMonitorBrokerHost.trim();
  piMonitorDeviceId.trim();
  piMonitorCommandTarget.trim();

  if (piMonitorBrokerHost.length() == 0) {
    piMonitorStatus = "Missing mqtt_host.";
    Serial.printf("Pi Monitor: %s has no mqtt_host value.\n", PI_CONFIG_PATH);
    return false;
  }

  if (piMonitorDeviceId.length() == 0) {
    piMonitorDeviceId = PI_MONITOR_DEFAULT_DEVICE_ID;
  }

  if (piMonitorCommandTarget.length() == 0) {
    piMonitorCommandStatus = "No command target.";
  } else if (piMonitorCommandCount == 0) {
    piMonitorCommandStatus = String("C read ") + piMonitorCommandTarget;
  }

  piMonitorConfigLoaded = true;
  return true;
}

bool connectPiMonitorMqtt() {
  if (WiFi.status() != WL_CONNECTED) {
    piMonitorStatus = "Use WiFi Connect.";
    Serial.println("Pi Monitor: Wi-Fi is not connected.");
    return false;
  }

  if (!readPiMonitorConfigFromSd()) {
    return false;
  }

  piMonitorStatus = "Connecting MQTT...";
  renderPiMonitor();

  if (piMonitorMqttClient.connected()) {
    piMonitorMqttClient.disconnect();
  }

  piMonitorMqttClient.setServer(piMonitorBrokerHost.c_str(), piMonitorBrokerPort);
  piMonitorMqttClient.setCallback(handlePiMonitorMessage);
  piMonitorMqttClient.setBufferSize(PI_MQTT_PACKET_BUFFER_SIZE);
  piMonitorMqttClient.setKeepAlive(PI_MQTT_KEEPALIVE_SECONDS);
  piMonitorMqttClient.setSocketTimeout(PI_MQTT_SOCKET_TIMEOUT_SECONDS);

  const String clientId = buildPiMonitorClientId();
  Serial.printf("Pi Monitor: connecting to MQTT %s:%u as %s\n",
                piMonitorBrokerHost.c_str(), piMonitorBrokerPort, clientId.c_str());

  if (!piMonitorMqttClient.connect(clientId.c_str())) {
    piMonitorStatus = String("MQTT failed: ") + mqttStateText(piMonitorMqttClient.state());
    Serial.printf("Pi Monitor: MQTT connect failed, state=%s\n",
                  mqttStateText(piMonitorMqttClient.state()));
    return false;
  }

  if (!piMonitorMqttClient.subscribe("home/#")) {
    piMonitorStatus = "MQTT sub failed.";
    Serial.println("Pi Monitor: MQTT subscription failed.");
    return false;
  }

  piMonitorStatus = "MQTT connected.";
  Serial.println("Pi Monitor: MQTT connected and subscribed to home/#.");
  return true;
}

void disconnectPiMonitorMqtt() {
  if (piMonitorMqttClient.connected()) {
    piMonitorMqttClient.disconnect();
  }

  piMonitorStatus = "MQTT disconnected.";
  Serial.println("Pi Monitor: MQTT disconnected.");
  renderPiMonitor();
}

bool publishPiMonitorReadNowCommand() {
  if (!piMonitorMqttClient.connected()) {
    piMonitorCommandStatus = "MQTT not connected.";
    Serial.println("Pi Monitor: read_now not sent; MQTT is not connected.");
    renderPiMonitor();
    return false;
  }

  if (piMonitorCommandTarget.length() == 0) {
    piMonitorCommandStatus = "No command target.";
    Serial.println("Pi Monitor: read_now not sent; command_target is missing.");
    renderPiMonitor();
    return false;
  }

  const String commandTopic = String("home/devices/") + piMonitorCommandTarget + "/commands";

  Serial.printf("Pi Monitor: publishing read_now to %s\n", commandTopic.c_str());
  const bool published =
      piMonitorMqttClient.publish(commandTopic.c_str(), PI_MONITOR_READ_NOW_PAYLOAD);

  if (published) {
    ++piMonitorCommandCount;
    piMonitorCommandStatus = String("read_now sent ") + String(piMonitorCommandCount);
    Serial.println("Pi Monitor: read_now command published.");
  } else {
    piMonitorCommandStatus = "read_now failed.";
    Serial.println("Pi Monitor: read_now publish failed.");
  }

  renderPiMonitor();
  return published;
}

void stopPiMonitor() {
  if (piMonitorMqttClient.connected()) {
    piMonitorMqttClient.disconnect();
    Serial.println("Pi Monitor: stopped.");
  }
}

void servicePiMonitor() {
  if (WiFi.status() != WL_CONNECTED) {
    if (piMonitorMqttClient.connected()) {
      piMonitorMqttClient.disconnect();
    }

    if (piMonitorStatus != "Use WiFi Connect.") {
      piMonitorStatus = "Use WiFi Connect.";
      renderPiMonitor();
    }
    return;
  }

  if (piMonitorMqttClient.connected()) {
    piMonitorMqttClient.loop();
  }
}

void clearPiMonitorDevices() {
  for (int i = 0; i < MAX_PI_MONITOR_DEVICES; ++i) {
    piMonitorDevices[i] = PiMonitorDevice();
  }

  piMonitorLastTopic = "";
  piMonitorLastPayload = "";
  piMonitorLastResponseDevice = "";
  piMonitorLastResponseSummary = "";
  piMonitorResponseCount = 0;
  piMonitorMessageCount = 0;
  Serial.println("Pi Monitor: device list cleared.");
}

void handlePiMonitorMessage(char* topic, byte* payload, unsigned int length) {
  String deviceId;
  String topicKind;

  String payloadText;
  const unsigned int safeLength = min(length, static_cast<unsigned int>(PI_MQTT_PACKET_BUFFER_SIZE - 1));
  payloadText.reserve(safeLength);
  for (unsigned int i = 0; i < safeLength; ++i) {
    payloadText += static_cast<char>(payload[i]);
  }
  payloadText.trim();

  piMonitorLastTopic = String(topic).substring(0, 64);
  piMonitorLastPayload = payloadText.substring(0, 96);
  ++piMonitorMessageCount;

  if (!parsePiMonitorTopic(topic, deviceId, topicKind)) {
    piMonitorStatus = "MQTT connected.";
    Serial.printf("Pi Monitor: %s %s\n", piMonitorLastTopic.c_str(),
                  piMonitorLastPayload.substring(0, 80).c_str());

    if (currentScreen == Screen::PiMonitor) {
      renderPiMonitor();
    }
    return;
  }

  const int slot = findPiMonitorDeviceSlot(deviceId);
  PiMonitorDevice& device = piMonitorDevices[slot];
  device.active = true;
  device.id = deviceId;
  device.topicKind = topicKind;
  device.lastSeenMs = millis();

  if (topicKind == "availability") {
    device.availability = payloadText.substring(0, 12);
    device.summary = "availability";
  } else if (isPiMonitorResponseKind(topicKind)) {
    piMonitorLastResponseDevice = deviceId;
    piMonitorLastResponseSummary = summarizeResponsePayload(payloadText);
    ++piMonitorResponseCount;
    piMonitorCommandStatus = String("response ") + String(piMonitorResponseCount);
    device.summary = piMonitorLastResponseSummary;
    if (device.availability.length() == 0) {
      device.availability = "seen";
    }
  } else {
    device.summary = summarizeJsonPayload(topicKind, payloadText);
    if (device.availability.length() == 0) {
      device.availability = "seen";
    }
  }

  piMonitorStatus = "MQTT connected.";
  Serial.printf("Pi Monitor: %s %s %s\n", deviceId.c_str(), topicKind.c_str(),
                payloadText.substring(0, 80).c_str());

  if (currentScreen == Screen::PiMonitor) {
    renderPiMonitor();
  }
}
