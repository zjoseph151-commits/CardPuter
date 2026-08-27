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

String normalizedPiMonitorDeviceId() {
  String deviceId = piMonitorDeviceId;
  deviceId.trim();

  if (deviceId.length() == 0) {
    deviceId = PI_MONITOR_DEFAULT_DEVICE_ID;
  }

  return deviceId;
}

String buildPiMonitorDeviceTopic(const char* topicKind) {
  return String("home/devices/") + normalizedPiMonitorDeviceId() + "/" + topicKind;
}

String activePiMonitorCommandTarget() {
  String target = piMonitorSelectedCommandTarget;
  target.trim();

  if (target.length() == 0) {
    target = piMonitorCommandTarget;
    target.trim();
  }

  return target;
}

bool isPiMonitorOwnDevice(const String& deviceId) {
  return deviceId == normalizedPiMonitorDeviceId();
}

bool piMonitorTargetCandidateExists(String candidates[], int candidateCount,
                                    const String& candidate) {
  for (int i = 0; i < candidateCount; ++i) {
    if (candidates[i] == candidate) {
      return true;
    }
  }

  return false;
}

void addPiMonitorTargetCandidate(String candidates[], int& candidateCount,
                                 int maxCandidates, const String& candidate) {
  String trimmedCandidate = candidate;
  trimmedCandidate.trim();

  if (trimmedCandidate.length() == 0 || isPiMonitorOwnDevice(trimmedCandidate) ||
      candidateCount >= maxCandidates ||
      piMonitorTargetCandidateExists(candidates, candidateCount, trimmedCandidate)) {
    return;
  }

  candidates[candidateCount] = trimmedCandidate;
  ++candidateCount;
}

int buildPiMonitorTargetCandidates(String candidates[], int maxCandidates) {
  int candidateCount = 0;
  addPiMonitorTargetCandidate(candidates, candidateCount, maxCandidates,
                              piMonitorCommandTarget);

  for (int i = 0; i < MAX_PI_MONITOR_DEVICES; ++i) {
    if (piMonitorDevices[i].active) {
      addPiMonitorTargetCandidate(candidates, candidateCount, maxCandidates,
                                  piMonitorDevices[i].id);
    }
  }

  return candidateCount;
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

bool hasTopicSegment(const String& topicKind, const char* segment) {
  const String segmentText(segment);
  const String prefix = segmentText + "/";
  const String suffix = String("/") + segmentText;
  const String middle = suffix + "/";
  return topicKind == segmentText || topicKind.startsWith(prefix) ||
         topicKind.endsWith(suffix) || topicKind.indexOf(middle) >= 0;
}

bool isPiMonitorResponseKind(const String& topicKind) {
  return hasTopicSegment(topicKind, "responses") ||
         hasTopicSegment(topicKind, "response") ||
         topicKind == "command_response" ||
         topicKind == "command_result" ||
         topicKind.endsWith("_response") ||
         topicKind.endsWith("_result");
}

bool isPiMonitorCommandTargetUpdate(const String& deviceId, const String& topicKind) {
  const String target = activePiMonitorCommandTarget();
  if (target.length() == 0 || deviceId != target ||
      piMonitorCommandCount == 0 || piMonitorLastCommandSentMs == 0) {
    return false;
  }

  if (topicKind == "availability" || topicKind == "commands" ||
      topicKind.startsWith("commands/")) {
    return false;
  }

  return millis() - piMonitorLastCommandSentMs <= PI_MONITOR_RESPONSE_WINDOW_MS;
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

bool buildPiMonitorStatusPayload(char* payload, size_t payloadSize) {
  StaticJsonDocument<384> document;
  const String statusDeviceId = normalizedPiMonitorDeviceId();
  document["device"] = statusDeviceId;
  document["firmware_version"] = FIRMWARE_VERSION;
  document["uptime_ms"] = millis();
  document["wifi_rssi"] = WiFi.RSSI();
  document["free_heap"] = ESP.getFreeHeap();

  if (lastBatteryVoltageMv > 0) {
    document["battery_mv"] = lastBatteryVoltageMv;
  }

  if (lastBatteryLevel >= 0 && isBatteryLevelDisplayable()) {
    document["battery_percent"] = lastBatteryLevel;
  }

  document["external_power"] = isExternalPowerPresent();
  document["charging"] = isFilteredCharging();

  return serializeJson(document, payload, payloadSize) > 0;
}

bool publishPiMonitorAvailability(const char* availability) {
  if (!piMonitorMqttClient.connected()) {
    return false;
  }

  const String availabilityTopic = buildPiMonitorDeviceTopic("availability");
  const bool published =
      piMonitorMqttClient.publish(availabilityTopic.c_str(), availability, true);

  if (published) {
    Serial.printf("Pi Monitor: published availability %s to %s\n", availability,
                  availabilityTopic.c_str());
  } else {
    Serial.printf("Pi Monitor: availability publish failed for %s\n",
                  availabilityTopic.c_str());
  }

  return published;
}

bool publishPiMonitorStatus() {
  if (!piMonitorMqttClient.connected()) {
    return false;
  }

  char statusPayload[384];
  if (!buildPiMonitorStatusPayload(statusPayload, sizeof(statusPayload))) {
    Serial.println("Pi Monitor: could not build status payload.");
    return false;
  }

  const String statusTopic = buildPiMonitorDeviceTopic("status");
  const bool published =
      piMonitorMqttClient.publish(statusTopic.c_str(), statusPayload, true);
  piMonitorLastStatusPublishMs = millis();

  if (published) {
    ++piMonitorStatusPublishCount;
    Serial.printf("Pi Monitor: published status %lu to %s\n",
                  static_cast<unsigned long>(piMonitorStatusPublishCount),
                  statusTopic.c_str());
  } else {
    Serial.printf("Pi Monitor: status publish failed for %s\n", statusTopic.c_str());
  }

  return published;
}

bool buildPiMonitorSetIntervalPayload(char* payload, size_t payloadSize) {
  StaticJsonDocument<96> document;
  document["command"] = "set_interval";
  document["seconds"] = selectedPiMonitorSetIntervalSeconds();
  return serializeJson(document, payload, payloadSize) > 0;
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
  String targetLine = activePiMonitorCommandTarget();
  if (targetLine.length() == 0) {
    targetLine = "none";
  }
  contentCanvas.printf("Tgt: %s\n", targetLine.substring(0, 22).c_str());
  contentCanvas.printf("Cmd: %s\n", piMonitorCommandStatus.substring(0, 22).c_str());
  if (piMonitorLastResponseSummary.length() > 0) {
    String responseLine = piMonitorLastResponseDevice + " " + piMonitorLastResponseSummary;
    contentCanvas.printf("Resp: %s\n", responseLine.substring(0, 22).c_str());
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
  prepareSharedSpiForSd();

  if (!SD.begin(SD_SPI_CS_PIN, SPI, SD_SPI_FREQUENCY)) {
    sharedSpiOwner = SHARED_SPI_OWNER_NONE;
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
  piMonitorSelectedCommandTarget = "";

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
  piMonitorSelectedCommandTarget = piMonitorCommandTarget;

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
    piMonitorCommandStatus =
        String("C read I") + String(selectedPiMonitorSetIntervalSeconds()) + " S set";
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
    publishPiMonitorAvailability("offline");
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

  const String availabilityTopic = buildPiMonitorDeviceTopic("availability");
  if (!piMonitorMqttClient.connect(clientId.c_str(), availabilityTopic.c_str(), 0, true,
                                   "offline")) {
    piMonitorStatus = String("MQTT failed: ") + mqttStateText(piMonitorMqttClient.state());
    Serial.printf("Pi Monitor: MQTT connect failed, state=%s\n",
                  mqttStateText(piMonitorMqttClient.state()));
    return false;
  }

  if (!piMonitorMqttClient.subscribe("home/#")) {
    piMonitorStatus = "MQTT sub failed.";
    Serial.println("Pi Monitor: MQTT subscription failed.");
    publishPiMonitorAvailability("offline");
    piMonitorMqttClient.disconnect();
    return false;
  }

  piMonitorStatus = "MQTT connected.";
  Serial.println("Pi Monitor: MQTT connected and subscribed to home/#.");
  publishPiMonitorAvailability("online");
  publishPiMonitorStatus();
  return true;
}

void disconnectPiMonitorMqtt() {
  if (piMonitorMqttClient.connected()) {
    publishPiMonitorAvailability("offline");
    piMonitorMqttClient.disconnect();
  }

  piMonitorStatus = "MQTT disconnected.";
  Serial.println("Pi Monitor: MQTT disconnected.");
  renderPiMonitor();
}

uint16_t selectedPiMonitorSetIntervalSeconds() {
  if (piMonitorSetIntervalIndex < 0 ||
      piMonitorSetIntervalIndex >= PI_MONITOR_SET_INTERVAL_OPTION_COUNT) {
    piMonitorSetIntervalIndex = 1;
  }

  return PI_MONITOR_SET_INTERVAL_OPTIONS_SECONDS[piMonitorSetIntervalIndex];
}

void cyclePiMonitorCommandTarget() {
  String candidates[MAX_PI_MONITOR_DEVICES + 1];
  const int candidateCount =
      buildPiMonitorTargetCandidates(candidates, MAX_PI_MONITOR_DEVICES + 1);

  if (candidateCount == 0) {
    piMonitorSelectedCommandTarget = "";
    piMonitorCommandStatus = "No command target.";
    Serial.println("Pi Monitor: no command targets to select.");
    renderPiMonitor();
    return;
  }

  const String currentTarget = activePiMonitorCommandTarget();
  int currentIndex = -1;
  for (int i = 0; i < candidateCount; ++i) {
    if (candidates[i] == currentTarget) {
      currentIndex = i;
      break;
    }
  }

  const int nextIndex = (currentIndex + 1) % candidateCount;
  piMonitorSelectedCommandTarget = candidates[nextIndex];
  piMonitorCommandStatus =
      String("target ") + piMonitorSelectedCommandTarget.substring(0, 14);
  Serial.printf("Pi Monitor: selected command target %s\n",
                piMonitorSelectedCommandTarget.c_str());
  renderPiMonitor();
}

void cyclePiMonitorSetInterval() {
  piMonitorSetIntervalIndex =
      (piMonitorSetIntervalIndex + 1) % PI_MONITOR_SET_INTERVAL_OPTION_COUNT;
  piMonitorCommandStatus =
      String("interval ") + String(selectedPiMonitorSetIntervalSeconds()) + "s S send";
  Serial.printf("Pi Monitor: selected set_interval %us\n",
                selectedPiMonitorSetIntervalSeconds());
  renderPiMonitor();
}

bool publishPiMonitorReadNowCommand() {
  if (!piMonitorMqttClient.connected()) {
    piMonitorCommandStatus = "MQTT not connected.";
    Serial.println("Pi Monitor: read_now not sent; MQTT is not connected.");
    renderPiMonitor();
    return false;
  }

  const String commandTarget = activePiMonitorCommandTarget();
  if (commandTarget.length() == 0) {
    piMonitorCommandStatus = "No command target.";
    Serial.println("Pi Monitor: read_now not sent; command_target is missing.");
    renderPiMonitor();
    return false;
  }

  const String commandTopic = String("home/devices/") + commandTarget + "/commands";

  Serial.printf("Pi Monitor: publishing read_now to %s\n", commandTopic.c_str());
  const bool published =
      piMonitorMqttClient.publish(commandTopic.c_str(), PI_MONITOR_READ_NOW_PAYLOAD);

  if (published) {
    ++piMonitorCommandCount;
    piMonitorLastCommandSentMs = millis();
    piMonitorLastResponseDevice = commandTarget;
    piMonitorLastResponseSummary = "";
    piMonitorCommandStatus = String("read_now sent ") + String(piMonitorCommandCount);
    Serial.println("Pi Monitor: read_now command published.");
  } else {
    piMonitorCommandStatus = "read_now failed.";
    Serial.println("Pi Monitor: read_now publish failed.");
  }

  renderPiMonitor();
  return published;
}

bool publishPiMonitorSetIntervalCommand() {
  if (!piMonitorMqttClient.connected()) {
    piMonitorCommandStatus = "MQTT not connected.";
    Serial.println("Pi Monitor: set_interval not sent; MQTT is not connected.");
    renderPiMonitor();
    return false;
  }

  const String commandTarget = activePiMonitorCommandTarget();
  if (commandTarget.length() == 0) {
    piMonitorCommandStatus = "No command target.";
    Serial.println("Pi Monitor: set_interval not sent; command_target is missing.");
    renderPiMonitor();
    return false;
  }

  char commandPayload[96];
  if (!buildPiMonitorSetIntervalPayload(commandPayload, sizeof(commandPayload))) {
    piMonitorCommandStatus = "set payload failed.";
    Serial.println("Pi Monitor: set_interval payload build failed.");
    renderPiMonitor();
    return false;
  }

  const String commandTopic = String("home/devices/") + commandTarget + "/commands";

  Serial.printf("Pi Monitor: publishing set_interval to %s: %s\n",
                commandTopic.c_str(), commandPayload);
  const bool published = piMonitorMqttClient.publish(commandTopic.c_str(), commandPayload);

  if (published) {
    ++piMonitorCommandCount;
    ++piMonitorSetIntervalCommandCount;
    piMonitorLastCommandSentMs = millis();
    piMonitorLastResponseDevice = commandTarget;
    piMonitorLastResponseSummary = "";
    piMonitorCommandStatus =
        String("set ") + String(selectedPiMonitorSetIntervalSeconds()) + "s sent";
    Serial.println("Pi Monitor: set_interval command published.");
  } else {
    piMonitorCommandStatus = "set_interval failed.";
    Serial.println("Pi Monitor: set_interval publish failed.");
  }

  renderPiMonitor();
  return published;
}

void stopPiMonitor() {
  if (piMonitorMqttClient.connected()) {
    publishPiMonitorAvailability("offline");
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
    if (millis() - piMonitorLastStatusPublishMs >=
        PI_MONITOR_STATUS_PUBLISH_INTERVAL_MS) {
      publishPiMonitorStatus();
    }
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
  piMonitorLastCommandSentMs = 0;
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

  const bool responseTopic = isPiMonitorResponseKind(topicKind);
  const bool commandTargetUpdate = isPiMonitorCommandTargetUpdate(deviceId, topicKind);

  if (topicKind == "availability") {
    device.availability = payloadText.substring(0, 12);
    device.summary = "availability";
  } else if (responseTopic || commandTargetUpdate) {
    piMonitorLastResponseDevice = deviceId;
    piMonitorLastResponseSummary =
        responseTopic ? summarizeResponsePayload(payloadText)
                      : summarizeJsonPayload(topicKind, payloadText);
    ++piMonitorResponseCount;
    piMonitorCommandStatus =
        responseTopic ? String("response ") + String(piMonitorResponseCount)
                      : String("target update ") + String(piMonitorResponseCount);
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
