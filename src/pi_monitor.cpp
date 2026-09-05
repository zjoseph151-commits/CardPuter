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

struct PiMonitorCommandProfileDef {
  const char* key;
  const char* label;
  const PiMonitorCommandDef* commands;
  uint8_t commandCount;
};

struct PiMonitorKnownProjectDef {
  const char* id;
  const char* label;
  const char* profile;
};

struct PiMonitorProjectListItem {
  bool diagnostics = false;
  bool discovered = false;
  String id;
  String label;
  String profile;
  String availability;
  String summary;
};

static constexpr PiMonitorCommandDef PI_MONITOR_BASIC_COMMANDS[] = {
    {"read_now", PiMonitorCommandKind::ReadNow},
    {"set_interval", PiMonitorCommandKind::SetInterval},
};

static constexpr PiMonitorCommandProfileDef PI_MONITOR_COMMAND_PROFILES[] = {
    {PI_MONITOR_DEFAULT_COMMAND_PROFILE, "Basic", PI_MONITOR_BASIC_COMMANDS,
     static_cast<uint8_t>(sizeof(PI_MONITOR_BASIC_COMMANDS) /
                          sizeof(PI_MONITOR_BASIC_COMMANDS[0]))},
};

static constexpr PiMonitorKnownProjectDef PI_MONITOR_KNOWN_PROJECTS[] = {
    {"esp32-c3-test", "ESP32-C3 Test", PI_MONITOR_DEFAULT_COMMAND_PROFILE},
};

constexpr int PI_MONITOR_PROJECT_ITEM_LIMIT =
    1 + MAX_PI_MONITOR_PROJECTS + MAX_PI_MONITOR_DEVICES;

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
  const String selectedProject = activePiMonitorProjectId();
  if (selectedProject.length() > 0) {
    return selectedProject;
  }

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

String knownPiMonitorProjectLabel(const String& id) {
  for (size_t i = 0;
       i < sizeof(PI_MONITOR_KNOWN_PROJECTS) /
               sizeof(PI_MONITOR_KNOWN_PROJECTS[0]);
       ++i) {
    if (id == PI_MONITOR_KNOWN_PROJECTS[i].id) {
      return PI_MONITOR_KNOWN_PROJECTS[i].label;
    }
  }

  return "";
}

String knownPiMonitorProjectProfile(const String& id) {
  for (size_t i = 0;
       i < sizeof(PI_MONITOR_KNOWN_PROJECTS) /
               sizeof(PI_MONITOR_KNOWN_PROJECTS[0]);
       ++i) {
    if (id == PI_MONITOR_KNOWN_PROJECTS[i].id) {
      return PI_MONITOR_KNOWN_PROJECTS[i].profile;
    }
  }

  return "";
}

String normalizedPiMonitorProjectLabel(const String& id, const String& label) {
  String trimmedLabel = label;
  trimmedLabel.trim();
  if (trimmedLabel.length() > 0) {
    return trimmedLabel;
  }

  const String knownLabel = knownPiMonitorProjectLabel(id);
  return knownLabel.length() > 0 ? knownLabel : id;
}

String normalizedPiMonitorProjectProfile(const String& id, const String& profile) {
  String trimmedProfile = profile;
  trimmedProfile.trim();
  if (trimmedProfile.length() > 0) {
    return trimmedProfile;
  }

  const String knownProfile = knownPiMonitorProjectProfile(id);
  return knownProfile.length() > 0 ? knownProfile
                                   : PI_MONITOR_DEFAULT_COMMAND_PROFILE;
}

int findExistingPiMonitorDeviceSlot(const String& deviceId) {
  for (int i = 0; i < MAX_PI_MONITOR_DEVICES; ++i) {
    if (piMonitorDevices[i].active && piMonitorDevices[i].id == deviceId) {
      return i;
    }
  }

  return -1;
}

void clearPiMonitorProjectConfigs() {
  for (int i = 0; i < MAX_PI_MONITOR_PROJECTS; ++i) {
    piMonitorProjects[i] = PiMonitorProjectConfig();
  }
}

bool piMonitorProjectConfigExists(const String& id) {
  for (int i = 0; i < MAX_PI_MONITOR_PROJECTS; ++i) {
    if (piMonitorProjects[i].active && piMonitorProjects[i].id == id) {
      return true;
    }
  }

  return false;
}

void addPiMonitorProjectConfig(const String& id, const String& label,
                               const String& profile) {
  String trimmedId = id;
  trimmedId.trim();
  if (trimmedId.length() == 0 || isPiMonitorOwnDevice(trimmedId) ||
      piMonitorProjectConfigExists(trimmedId)) {
    return;
  }

  for (int i = 0; i < MAX_PI_MONITOR_PROJECTS; ++i) {
    if (!piMonitorProjects[i].active) {
      piMonitorProjects[i].active = true;
      piMonitorProjects[i].id = trimmedId.substring(0, 32);
      piMonitorProjects[i].label =
          normalizedPiMonitorProjectLabel(trimmedId, label).substring(0, 32);
      piMonitorProjects[i].profile =
          normalizedPiMonitorProjectProfile(trimmedId, profile).substring(0, 16);
      return;
    }
  }
}

void parsePiMonitorProjectConfig(const String& value) {
  String parts[3];
  const char separator = value.indexOf('|') >= 0 ? '|' : ',';
  int partIndex = 0;
  int start = 0;

  while (partIndex < 3 && start <= value.length()) {
    int end = value.indexOf(separator, start);
    if (end < 0) {
      end = value.length();
    }

    parts[partIndex] = value.substring(start, end);
    parts[partIndex].trim();
    ++partIndex;
    start = end + 1;
  }

  addPiMonitorProjectConfig(parts[0], parts[1], parts[2]);
}

bool piMonitorProjectItemExists(PiMonitorProjectListItem items[], int itemCount,
                                const String& id) {
  for (int i = 0; i < itemCount; ++i) {
    if (!items[i].diagnostics && items[i].id == id) {
      return true;
    }
  }

  return false;
}

void addPiMonitorProjectListItem(PiMonitorProjectListItem items[], int& itemCount,
                                 int maxItems,
                                 const PiMonitorProjectListItem& item) {
  if (itemCount >= maxItems ||
      (!item.diagnostics && item.id.length() == 0) ||
      (!item.diagnostics && piMonitorProjectItemExists(items, itemCount, item.id))) {
    return;
  }

  items[itemCount] = item;
  ++itemCount;
}

void addPiMonitorConfiguredProjects(PiMonitorProjectListItem items[],
                                    int& itemCount, int maxItems) {
  for (int i = 0; i < MAX_PI_MONITOR_PROJECTS; ++i) {
    if (!piMonitorProjects[i].active) {
      continue;
    }

    PiMonitorProjectListItem item;
    item.id = piMonitorProjects[i].id;
    item.label = normalizedPiMonitorProjectLabel(item.id, piMonitorProjects[i].label);
    item.profile =
        normalizedPiMonitorProjectProfile(item.id, piMonitorProjects[i].profile);

    const int deviceSlot = findExistingPiMonitorDeviceSlot(item.id);
    if (deviceSlot >= 0) {
      item.availability = piMonitorDevices[deviceSlot].availability;
      item.summary = piMonitorDevices[deviceSlot].summary;
    }

    addPiMonitorProjectListItem(items, itemCount, maxItems, item);
  }
}

void addPiMonitorDiscoveredProjects(PiMonitorProjectListItem items[],
                                    int& itemCount, int maxItems) {
  for (int i = 0; i < MAX_PI_MONITOR_DEVICES; ++i) {
    if (!piMonitorDevices[i].active || isPiMonitorOwnDevice(piMonitorDevices[i].id)) {
      continue;
    }

    PiMonitorProjectListItem item;
    item.discovered = true;
    item.id = piMonitorDevices[i].id;
    item.label = normalizedPiMonitorProjectLabel(item.id, "");
    item.profile = normalizedPiMonitorProjectProfile(item.id, "");
    item.availability = piMonitorDevices[i].availability;
    item.summary = piMonitorDevices[i].summary;
    addPiMonitorProjectListItem(items, itemCount, maxItems, item);
  }
}

int buildPiMonitorProjectItems(PiMonitorProjectListItem items[], int maxItems) {
  if (maxItems <= 0) {
    return 0;
  }

  int itemCount = 0;
  PiMonitorProjectListItem diagnostics;
  diagnostics.diagnostics = true;
  diagnostics.label = "Home / Diagnostics";
  addPiMonitorProjectListItem(items, itemCount, maxItems, diagnostics);
  addPiMonitorConfiguredProjects(items, itemCount, maxItems);
  addPiMonitorDiscoveredProjects(items, itemCount, maxItems);
  return itemCount;
}

int piMonitorProjectItemCount() {
  PiMonitorProjectListItem items[PI_MONITOR_PROJECT_ITEM_LIMIT];
  return buildPiMonitorProjectItems(items, PI_MONITOR_PROJECT_ITEM_LIMIT);
}

void clampPiMonitorProjectSelection() {
  const int itemCount = piMonitorProjectItemCount();
  if (itemCount <= 0) {
    selectedPiMonitorProjectIndex = 0;
    piMonitorProjectScrollOffset = 0;
    return;
  }

  if (selectedPiMonitorProjectIndex < 0) {
    selectedPiMonitorProjectIndex = itemCount - 1;
  } else if (selectedPiMonitorProjectIndex >= itemCount) {
    selectedPiMonitorProjectIndex = 0;
  }

  if (selectedPiMonitorProjectIndex < piMonitorProjectScrollOffset) {
    piMonitorProjectScrollOffset = selectedPiMonitorProjectIndex;
  } else if (selectedPiMonitorProjectIndex >=
             piMonitorProjectScrollOffset + PI_MONITOR_PROJECT_VISIBLE_ROWS) {
    piMonitorProjectScrollOffset =
        selectedPiMonitorProjectIndex - PI_MONITOR_PROJECT_VISIBLE_ROWS + 1;
  }

  piMonitorProjectScrollOffset =
      constrain(piMonitorProjectScrollOffset, 0,
                max(0, itemCount - PI_MONITOR_PROJECT_VISIBLE_ROWS));
}

bool selectedPiMonitorProjectItem(PiMonitorProjectListItem& item) {
  PiMonitorProjectListItem items[PI_MONITOR_PROJECT_ITEM_LIMIT];
  const int itemCount =
      buildPiMonitorProjectItems(items, PI_MONITOR_PROJECT_ITEM_LIMIT);
  if (itemCount <= 0) {
    return false;
  }

  clampPiMonitorProjectSelection();
  if (selectedPiMonitorProjectIndex < 0 ||
      selectedPiMonitorProjectIndex >= itemCount) {
    return false;
  }

  item = items[selectedPiMonitorProjectIndex];
  return true;
}

const PiMonitorCommandProfileDef* findPiMonitorCommandProfile(
    const String& profile) {
  String trimmedProfile = profile;
  trimmedProfile.trim();
  if (trimmedProfile.length() == 0) {
    trimmedProfile = PI_MONITOR_DEFAULT_COMMAND_PROFILE;
  }

  for (size_t i = 0;
       i < sizeof(PI_MONITOR_COMMAND_PROFILES) /
               sizeof(PI_MONITOR_COMMAND_PROFILES[0]);
       ++i) {
    if (trimmedProfile == PI_MONITOR_COMMAND_PROFILES[i].key) {
      return &PI_MONITOR_COMMAND_PROFILES[i];
    }
  }

  return &PI_MONITOR_COMMAND_PROFILES[0];
}

const PiMonitorCommandProfileDef* selectedPiMonitorCommandProfile() {
  PiMonitorProjectListItem project;
  if (!selectedPiMonitorProjectItem(project) || project.diagnostics) {
    return &PI_MONITOR_COMMAND_PROFILES[0];
  }

  return findPiMonitorCommandProfile(project.profile);
}

uint8_t selectedPiMonitorCommandCount() {
  const PiMonitorCommandProfileDef* profile = selectedPiMonitorCommandProfile();
  return profile == nullptr ? 0 : profile->commandCount;
}

void clampPiMonitorCommandSelection() {
  const uint8_t commandCount = selectedPiMonitorCommandCount();
  if (commandCount == 0) {
    selectedPiMonitorCommandIndex = 0;
    piMonitorCommandScrollOffset = 0;
    return;
  }

  if (selectedPiMonitorCommandIndex < 0) {
    selectedPiMonitorCommandIndex = commandCount - 1;
  } else if (selectedPiMonitorCommandIndex >= commandCount) {
    selectedPiMonitorCommandIndex = 0;
  }

  if (selectedPiMonitorCommandIndex < piMonitorCommandScrollOffset) {
    piMonitorCommandScrollOffset = selectedPiMonitorCommandIndex;
  } else if (selectedPiMonitorCommandIndex >=
             piMonitorCommandScrollOffset + PI_MONITOR_COMMAND_VISIBLE_ROWS) {
    piMonitorCommandScrollOffset =
        selectedPiMonitorCommandIndex - PI_MONITOR_COMMAND_VISIBLE_ROWS + 1;
  }

  piMonitorCommandScrollOffset =
      constrain(piMonitorCommandScrollOffset, 0,
                max(0, static_cast<int>(commandCount) -
                           PI_MONITOR_COMMAND_VISIBLE_ROWS));
}

const PiMonitorCommandDef* selectedPiMonitorCommandDef() {
  const PiMonitorCommandProfileDef* profile = selectedPiMonitorCommandProfile();
  if (profile == nullptr || profile->commandCount == 0) {
    return nullptr;
  }

  clampPiMonitorCommandSelection();
  if (selectedPiMonitorCommandIndex < 0 ||
      selectedPiMonitorCommandIndex >= profile->commandCount) {
    return nullptr;
  }

  return &profile->commands[selectedPiMonitorCommandIndex];
}

String piMonitorCommandDisplayLabel(const PiMonitorCommandDef& command) {
  if (command.kind == PiMonitorCommandKind::SetInterval) {
    return String(command.label) + " " +
           String(selectedPiMonitorSetIntervalSeconds()) + "s";
  }

  return command.label;
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

void clearPiMonitorMessages() {
  for (int i = 0; i < MAX_PI_MONITOR_MESSAGES; ++i) {
    piMonitorMessages[i] = PiMonitorMessage();
  }

  piMonitorMessageWriteIndex = 0;
  piMonitorStoredMessageCount = 0;
}

void recordPiMonitorMessage(const String& topic, const String& payload,
                            const String& deviceId, const String& topicKind) {
  PiMonitorMessage& message = piMonitorMessages[piMonitorMessageWriteIndex];
  message.active = true;
  message.topic = topic.substring(0, PI_MONITOR_MESSAGE_TOPIC_MAX_CHARS);
  message.payload = payload.substring(0, PI_MONITOR_MESSAGE_PAYLOAD_MAX_CHARS);
  message.deviceId = deviceId.substring(0, 32);
  message.topicKind = topicKind.substring(0, 32);
  message.receivedMs = millis();
  message.sequence = piMonitorMessageCount;

  piMonitorMessageWriteIndex =
      (piMonitorMessageWriteIndex + 1) % MAX_PI_MONITOR_MESSAGES;
  if (piMonitorStoredMessageCount < MAX_PI_MONITOR_MESSAGES) {
    ++piMonitorStoredMessageCount;
  }
}

bool piMonitorMessageMatchesView(const PiMonitorMessage& message) {
  if (!message.active) {
    return false;
  }

  if (piMonitorView == PiMonitorView::Diagnostics) {
    return true;
  }

  if (piMonitorView != PiMonitorView::ProjectCommands &&
      piMonitorView != PiMonitorView::ProjectList) {
    return false;
  }

  PiMonitorProjectListItem project;
  if (!selectedPiMonitorProjectItem(project)) {
    return false;
  }

  if (project.diagnostics) {
    return true;
  }

  return project.id.length() > 0 && message.deviceId == project.id;
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

String piMonitorProjectRowLabel(const PiMonitorProjectListItem& item) {
  if (item.diagnostics) {
    return "Home / Diagnostics";
  }

  String row = item.label;
  if (row.length() == 0) {
    row = item.id;
  }

  String detail = item.availability;
  detail.trim();
  if (detail.length() == 0) {
    detail = item.summary;
  }
  detail.trim();

  if (detail.length() > 0) {
    row += " ";
    row += detail.substring(0, 9);
  } else if (item.discovered) {
    row += " seen";
  }

  return row.substring(0, 27);
}

void drawPiMonitorSelectableRow(int visibleIndex, bool selected, const String& text) {
  const int rowY = 36 + (visibleIndex * 14);
  if (selected) {
    contentCanvas.fillRect(6, rowY - 2, M5Cardputer.Display.width() - 12, 14,
                           DARKGREEN);
    contentCanvas.setTextColor(WHITE, DARKGREEN);
    contentCanvas.setCursor(12, rowY);
    contentCanvas.print("> ");
  } else {
    contentCanvas.setTextColor(WHITE, BLACK);
    contentCanvas.setCursor(22, rowY);
  }

  contentCanvas.print(text);
  contentCanvas.setTextColor(WHITE, BLACK);
}

void renderPiMonitorProjectListView() {
  beginContentDraw();
  clampPiMonitorProjectSelection();

  PiMonitorProjectListItem items[PI_MONITOR_PROJECT_ITEM_LIMIT];
  const int itemCount =
      buildPiMonitorProjectItems(items, PI_MONITOR_PROJECT_ITEM_LIMIT);
  const int projectCount = max(0, itemCount - 1);

  contentCanvas.printf("MQTT:%s Msg:%lu P:%d\n",
                       piMonitorMqttClient.connected() ? "on" : "off",
                       static_cast<unsigned long>(piMonitorMessageCount),
                       projectCount);
  if (piMonitorBrokerHost.length() > 0) {
    String brokerLine = piMonitorBrokerHost + ":" + String(piMonitorBrokerPort);
    contentCanvas.printf("Pi: %s\n", brokerLine.substring(0, 25).c_str());
  } else {
    contentCanvas.println("Config: /config/pi.txt");
  }

  const int visibleCount = min(PI_MONITOR_PROJECT_VISIBLE_ROWS, itemCount);
  for (int visibleIndex = 0; visibleIndex < visibleCount; ++visibleIndex) {
    const int itemIndex = piMonitorProjectScrollOffset + visibleIndex;
    if (itemIndex >= itemCount) {
      break;
    }

    drawPiMonitorSelectableRow(visibleIndex, itemIndex == selectedPiMonitorProjectIndex,
                               piMonitorProjectRowLabel(items[itemIndex]));
  }

  if (itemCount == 1) {
    contentCanvas.setTextColor(DARKGREY, BLACK);
    contentCanvas.setCursor(8, 104);
    contentCanvas.print("Waiting for projects");
    contentCanvas.setTextColor(WHITE, BLACK);
  }

  commitContentDraw();
}

void renderPiMonitorCommandView() {
  beginContentDraw();
  clampPiMonitorCommandSelection();

  PiMonitorProjectListItem project;
  if (!selectedPiMonitorProjectItem(project) || project.diagnostics) {
    contentCanvas.println("No project selected.");
    contentCanvas.println("Back to project list.");
    commitContentDraw();
    return;
  }

  const PiMonitorCommandProfileDef* profile = selectedPiMonitorCommandProfile();
  const uint8_t commandCount = profile == nullptr ? 0 : profile->commandCount;
  contentCanvas.printf("Project: %s\n", project.label.substring(0, 21).c_str());
  contentCanvas.printf("MQTT:%s %s\n", piMonitorMqttClient.connected() ? "on" : "off",
                       piMonitorCommandStatus.substring(0, 19).c_str());

  if (commandCount == 0) {
    contentCanvas.println("No commands.");
    commitContentDraw();
    return;
  }

  const int visibleCount =
      min(PI_MONITOR_COMMAND_VISIBLE_ROWS, static_cast<int>(commandCount));
  for (int visibleIndex = 0; visibleIndex < visibleCount; ++visibleIndex) {
    const int commandIndex = piMonitorCommandScrollOffset + visibleIndex;
    if (commandIndex >= commandCount) {
      break;
    }

    drawPiMonitorSelectableRow(
        visibleIndex, commandIndex == selectedPiMonitorCommandIndex,
        piMonitorCommandDisplayLabel(profile->commands[commandIndex]));
  }

  commitContentDraw();
}

void renderPiMonitorDiagnosticsView() {
  beginContentDraw();

  contentCanvas.println("Home / Diagnostics");
  contentCanvas.printf("MQTT:%s State:%s\n",
                       piMonitorMqttClient.connected() ? "on" : "off",
                       mqttStateText(piMonitorMqttClient.state()));
  contentCanvas.printf("Msgs:%lu Devs:%d Stored:%lu\n",
                       static_cast<unsigned long>(piMonitorMessageCount),
                       countPiMonitorDevices(),
                       static_cast<unsigned long>(piMonitorStoredMessageCount));

  if (piMonitorBrokerHost.length() > 0) {
    String brokerLine = piMonitorBrokerHost + ":" + String(piMonitorBrokerPort);
    contentCanvas.printf("Pi: %s\n", brokerLine.substring(0, 24).c_str());
  } else {
    contentCanvas.println("Config: /config/pi.txt");
  }

  contentCanvas.printf("Status: %s\n", piMonitorStatus.substring(0, 20).c_str());
  if (piMonitorLastResponseSummary.length() > 0) {
    String responseLine = piMonitorLastResponseDevice + " " + piMonitorLastResponseSummary;
    contentCanvas.printf("Resp: %s\n", responseLine.substring(0, 22).c_str());
  }

  if (piMonitorLastTopic.length() > 0) {
    contentCanvas.printf("Last: %s\n",
                         compactTopic(piMonitorLastTopic).substring(0, 20).c_str());
    contentCanvas.printf("Pay: %s\n", piMonitorLastPayload.substring(0, 22).c_str());
  } else {
    contentCanvas.println(WiFi.status() == WL_CONNECTED ? "Waiting for MQTT data."
                                                        : "WiFi: use WiFi Connect");
  }

  contentCanvas.println("R retry D disconnect");
  commitContentDraw();
}

}  // namespace

void showPiMonitor() {
  piMonitorView = PiMonitorView::ProjectList;
  selectedPiMonitorProjectIndex = 0;
  piMonitorProjectScrollOffset = 0;
  selectedPiMonitorCommandIndex = 0;
  piMonitorCommandScrollOffset = 0;
  piMonitorSelectedCommandTarget = "";
  piMonitorStatus = "Reading config...";
  renderPiMonitor();
  connectPiMonitorMqtt();
  renderPiMonitor();
}

void renderPiMonitor() {
  if (piMonitorView == PiMonitorView::ProjectCommands) {
    renderPiMonitorCommandView();
  } else if (piMonitorView == PiMonitorView::Diagnostics) {
    renderPiMonitorDiagnosticsView();
  } else {
    renderPiMonitorProjectListView();
  }
}

const char* piMonitorViewLabel() {
  if (piMonitorView == PiMonitorView::ProjectCommands) {
    return "Commands";
  }
  if (piMonitorView == PiMonitorView::Diagnostics) {
    return "Diagnostics";
  }
  return "Projects";
}

String activePiMonitorProjectId() {
  PiMonitorProjectListItem project;
  if (!selectedPiMonitorProjectItem(project) || project.diagnostics) {
    return "";
  }

  return project.id;
}

String activePiMonitorProjectLabel() {
  PiMonitorProjectListItem project;
  if (!selectedPiMonitorProjectItem(project)) {
    return "No project";
  }

  return project.diagnostics ? String("Home / Diagnostics") : project.label;
}

bool latestPiMonitorOledMessage(PiMonitorMessage& latestMessage) {
  const uint32_t storedCount = min(piMonitorStoredMessageCount,
                                   static_cast<uint32_t>(MAX_PI_MONITOR_MESSAGES));

  for (uint32_t offset = 0; offset < storedCount; ++offset) {
    int messageIndex = piMonitorMessageWriteIndex - 1 - static_cast<int>(offset);
    while (messageIndex < 0) {
      messageIndex += MAX_PI_MONITOR_MESSAGES;
    }

    const PiMonitorMessage& message = piMonitorMessages[messageIndex];
    if (!piMonitorMessageMatchesView(message)) {
      continue;
    }

    latestMessage = message;
    return true;
  }

  return false;
}

String piMonitorLatestOledMessageText() {
  PiMonitorMessage message;
  if (!latestPiMonitorOledMessage(message)) {
    return "waiting for MQTT messages";
  }

  String text = message.topic;
  text.trim();

  String payload = message.payload;
  payload.trim();
  if (payload.length() > 0) {
    if (text.length() > 0) {
      text += "\n";
    }
    text += payload;
  }

  return text.length() > 0 ? text : String("empty MQTT message");
}

uint32_t piMonitorLatestOledMessageSequence() {
  PiMonitorMessage message;
  return latestPiMonitorOledMessage(message) ? message.sequence : 0;
}

uint32_t piMonitorFilteredMessageCount() {
  uint32_t count = 0;
  const uint32_t storedCount = min(piMonitorStoredMessageCount,
                                   static_cast<uint32_t>(MAX_PI_MONITOR_MESSAGES));

  for (uint32_t i = 0; i < storedCount; ++i) {
    if (piMonitorMessageMatchesView(piMonitorMessages[i])) {
      ++count;
    }
  }

  return count;
}

void movePiMonitorSelection(int direction) {
  if (direction == 0) {
    return;
  }

  if (piMonitorView == PiMonitorView::ProjectCommands) {
    selectedPiMonitorCommandIndex += direction;
    clampPiMonitorCommandSelection();
    renderPiMonitor();
    return;
  }

  if (piMonitorView == PiMonitorView::ProjectList) {
    selectedPiMonitorProjectIndex += direction;
    clampPiMonitorProjectSelection();

    PiMonitorProjectListItem item;
    if (selectedPiMonitorProjectItem(item) && !item.diagnostics) {
      piMonitorSelectedCommandTarget = item.id;
    }

    renderPiMonitor();
  }
}

void enterPiMonitorSelection() {
  if (piMonitorView == PiMonitorView::ProjectCommands) {
    publishSelectedPiMonitorCommand();
    return;
  }

  if (piMonitorView == PiMonitorView::Diagnostics) {
    connectPiMonitorMqtt();
    renderPiMonitor();
    return;
  }

  PiMonitorProjectListItem item;
  if (!selectedPiMonitorProjectItem(item)) {
    renderPiMonitor();
    return;
  }

  if (item.diagnostics) {
    piMonitorView = PiMonitorView::Diagnostics;
    renderPiMonitor();
    return;
  }

  piMonitorSelectedCommandTarget = item.id;
  selectedPiMonitorCommandIndex = 0;
  piMonitorCommandScrollOffset = 0;
  piMonitorCommandStatus = String("Project ") + item.label.substring(0, 14);
  piMonitorView = PiMonitorView::ProjectCommands;
  renderPiMonitor();
}

bool returnPiMonitorProjectList() {
  if (piMonitorView == PiMonitorView::ProjectList) {
    return false;
  }

  piMonitorView = PiMonitorView::ProjectList;
  renderPiMonitor();
  return true;
}

void cyclePiMonitorCommandOption(int direction) {
  const PiMonitorCommandDef* command = selectedPiMonitorCommandDef();
  if (command == nullptr || command->kind != PiMonitorCommandKind::SetInterval) {
    piMonitorCommandStatus = "No option here.";
    renderPiMonitor();
    return;
  }

  piMonitorSetIntervalIndex += direction < 0 ? -1 : 1;
  if (piMonitorSetIntervalIndex < 0) {
    piMonitorSetIntervalIndex = PI_MONITOR_SET_INTERVAL_OPTION_COUNT - 1;
  } else if (piMonitorSetIntervalIndex >= PI_MONITOR_SET_INTERVAL_OPTION_COUNT) {
    piMonitorSetIntervalIndex = 0;
  }

  piMonitorCommandStatus =
      String("interval ") + String(selectedPiMonitorSetIntervalSeconds()) + "s OK";
  Serial.printf("Pi Monitor: selected set_interval %us\n",
                selectedPiMonitorSetIntervalSeconds());
  renderPiMonitor();
}

bool publishSelectedPiMonitorCommand() {
  const PiMonitorCommandDef* command = selectedPiMonitorCommandDef();
  if (command == nullptr) {
    piMonitorCommandStatus = "No command.";
    renderPiMonitor();
    return false;
  }

  if (command->kind == PiMonitorCommandKind::ReadNow) {
    return publishPiMonitorReadNowCommand();
  }
  if (command->kind == PiMonitorCommandKind::SetInterval) {
    return publishPiMonitorSetIntervalCommand();
  }

  piMonitorCommandStatus = "Command blocked.";
  renderPiMonitor();
  return false;
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
  clearPiMonitorProjectConfigs();

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
    } else if (key == "project") {
      parsePiMonitorProjectConfig(trimmedConfigValue(line));
    }
  }

  configFile.close();
  piMonitorBrokerHost.trim();
  piMonitorDeviceId.trim();
  piMonitorCommandTarget.trim();
  piMonitorSelectedCommandTarget = piMonitorCommandTarget;
  addPiMonitorProjectConfig(piMonitorCommandTarget, "",
                            PI_MONITOR_DEFAULT_COMMAND_PROFILE);
  clampPiMonitorProjectSelection();

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
        String("C read I") + String(selectedPiMonitorSetIntervalSeconds()) +
        " OK set";
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
  PiMonitorProjectListItem items[PI_MONITOR_PROJECT_ITEM_LIMIT];
  const int itemCount =
      buildPiMonitorProjectItems(items, PI_MONITOR_PROJECT_ITEM_LIMIT);
  int nextIndex = -1;

  for (int step = 1; step <= itemCount; ++step) {
    const int candidateIndex = (selectedPiMonitorProjectIndex + step) % itemCount;
    if (!items[candidateIndex].diagnostics) {
      nextIndex = candidateIndex;
      break;
    }
  }

  if (nextIndex < 0) {
    piMonitorSelectedCommandTarget = "";
    piMonitorCommandStatus = "No command target.";
    Serial.println("Pi Monitor: no command targets to select.");
    renderPiMonitor();
    return;
  }

  selectedPiMonitorProjectIndex = nextIndex;
  selectedPiMonitorCommandIndex = 0;
  piMonitorCommandScrollOffset = 0;
  piMonitorSelectedCommandTarget = items[nextIndex].id;
  piMonitorCommandStatus =
      String("target ") + piMonitorSelectedCommandTarget.substring(0, 14);
  if (piMonitorView == PiMonitorView::Diagnostics) {
    piMonitorView = PiMonitorView::ProjectList;
  }
  clampPiMonitorProjectSelection();
  Serial.printf("Pi Monitor: selected command target %s\n",
                piMonitorSelectedCommandTarget.c_str());
  renderPiMonitor();
}

void cyclePiMonitorSetInterval() {
  piMonitorSetIntervalIndex =
      (piMonitorSetIntervalIndex + 1) % PI_MONITOR_SET_INTERVAL_OPTION_COUNT;
  piMonitorCommandStatus =
      String("interval ") + String(selectedPiMonitorSetIntervalSeconds()) + "s OK";
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
  clearPiMonitorMessages();
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

  const String fullTopic = String(topic);
  piMonitorLastTopic = fullTopic.substring(0, 64);
  piMonitorLastPayload = payloadText.substring(0, 96);
  ++piMonitorMessageCount;

  if (!parsePiMonitorTopic(topic, deviceId, topicKind)) {
    recordPiMonitorMessage(fullTopic, payloadText, "", "");
    piMonitorStatus = "MQTT connected.";
    Serial.printf("Pi Monitor: %s %s\n", piMonitorLastTopic.c_str(),
                  piMonitorLastPayload.substring(0, 80).c_str());

    if (currentScreen == Screen::PiMonitor) {
      renderPiMonitor();
    }
    return;
  }

  recordPiMonitorMessage(fullTopic, payloadText, deviceId, topicKind);
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
  clampPiMonitorProjectSelection();
  Serial.printf("Pi Monitor: %s %s %s\n", deviceId.c_str(), topicKind.c_str(),
                payloadText.substring(0, 80).c_str());

  if (currentScreen == Screen::PiMonitor) {
    renderPiMonitor();
  }
}
