#include "app.h"

namespace {

struct SdManagerFolderDef {
  const char* label;
  const char* path;
};

static constexpr SdManagerFolderDef SD_MANAGER_FOLDERS[] = {
    {"config", "/config"},
    {"env", "/env"},
    {"memos", "/memos"},
    {"tracks", BREADCRUMB_LOG_DIR},
};

constexpr int SD_MANAGER_FOLDER_COUNT =
    sizeof(SD_MANAGER_FOLDERS) / sizeof(SD_MANAGER_FOLDERS[0]);
constexpr int SD_MANAGER_TEXT_VISIBLE_ROWS = 4;
constexpr int SD_MANAGER_TEMP_SUFFIX_EXTRA = 5;
constexpr int SD_MANAGER_BACKUP_SUFFIX_EXTRA = 5;
constexpr const char* SD_MANAGER_WIFI_TEMPLATE_NAME = "wifi.txt";
constexpr const char* SD_MANAGER_PI_TEMPLATE_NAME = "pi.txt";

String clippedSdManagerText(const String& text, int maxChars) {
  return text.substring(0, maxChars);
}

String sdManagerFolderPath(int index) {
  if (index < 0 || index >= SD_MANAGER_FOLDER_COUNT) {
    return SD_MANAGER_FOLDERS[0].path;
  }

  return SD_MANAGER_FOLDERS[index].path;
}

String sdManagerFolderLabel(int index) {
  if (index < 0 || index >= SD_MANAGER_FOLDER_COUNT) {
    return SD_MANAGER_FOLDERS[0].label;
  }

  return SD_MANAGER_FOLDERS[index].label;
}

bool sdManagerInConfigFolder() {
  return sdManagerCurrentFolder == SD_MANAGER_FOLDERS[0].path;
}

bool sdManagerIsKnownFolder(const String& path) {
  for (int i = 0; i < SD_MANAGER_FOLDER_COUNT; ++i) {
    if (path == SD_MANAGER_FOLDERS[i].path) {
      return true;
    }
  }

  return false;
}

bool sdManagerPathStartsWithFolder(const String& path, const char* folder) {
  const String prefix = String(folder) + "/";
  return path == folder || path.startsWith(prefix);
}

bool sdManagerSafePath(const String& path) {
  if (!path.startsWith("/") || path.indexOf('\\') >= 0 ||
      path.indexOf("..") >= 0 || path.indexOf("//") >= 0) {
    return false;
  }

  for (int i = 0; i < SD_MANAGER_FOLDER_COUNT; ++i) {
    if (sdManagerPathStartsWithFolder(path, SD_MANAGER_FOLDERS[i].path)) {
      return true;
    }
  }

  return false;
}

bool sdManagerSafeFilePath(const String& path) {
  return sdManagerSafePath(path) && !sdManagerIsKnownFolder(path);
}

bool sdManagerAllowedNameChar(char c) {
  return isAlphaNumeric(c) || c == '_' || c == '-' || c == '.';
}

bool sdManagerAllowedValueChar(char c) {
  return c >= 32 && c <= 126;
}

bool sdManagerSafeFileName(const String& name) {
  if (name.length() == 0 || name.length() > SD_MANAGER_NAME_MAX_LENGTH ||
      name.startsWith(".") || name.indexOf("..") >= 0 ||
      name.endsWith(".") || name.indexOf('/') >= 0 ||
      name.indexOf('\\') >= 0) {
    return false;
  }

  for (int i = 0; i < name.length(); ++i) {
    if (!sdManagerAllowedNameChar(name.charAt(i))) {
      return false;
    }
  }

  return true;
}

String sdManagerBuildPath(const String& folder, const String& name) {
  return folder + "/" + name;
}

String sdManagerLowerExtension(const String& path) {
  const int slashIndex = path.lastIndexOf('/');
  const int dotIndex = path.lastIndexOf('.');
  if (dotIndex < 0 || dotIndex <= slashIndex ||
      dotIndex >= path.length() - 1) {
    return "";
  }

  String extension = path.substring(dotIndex);
  extension.toLowerCase();
  return extension;
}

bool sdManagerPathEquals(const String& path, const char* expectedPath) {
  String normalizedPath = path;
  String normalizedExpected = expectedPath;
  normalizedPath.toLowerCase();
  normalizedExpected.toLowerCase();
  return normalizedPath == normalizedExpected;
}

bool sdManagerIsTextExtension(const String& path) {
  const String extension = sdManagerLowerExtension(path);
  return extension == ".txt" || extension == ".csv" || extension == ".log" ||
         extension == ".json" || extension == ".cfg" || extension == ".ini" ||
         extension == ".md";
}

bool sdManagerCanSniffTextPath(const String& path) {
  return sdManagerLowerExtension(path).length() == 0 &&
         sdManagerPathStartsWithFolder(path, ENV_LOG_DIR);
}

bool sdManagerCanPreviewLargeTextPath(const String& path) {
  return sdManagerLowerExtension(path) == ".csv" ||
         sdManagerCanSniffTextPath(path);
}

bool sdManagerFileLooksText(File& file) {
  size_t sampleCount = 0;
  size_t nonTextCount = 0;

  while (file.available() && sampleCount < 256) {
    const int value = file.read();
    if (value < 0) {
      break;
    }

    ++sampleCount;
    if (value == 0) {
      nonTextCount = sampleCount;
      break;
    }

    if (value != '\n' && value != '\r' && value != '\t' &&
        (value < 32 || value > 126)) {
      ++nonTextCount;
    }
  }

  file.seek(0);
  return sampleCount == 0 || nonTextCount == 0 ||
         nonTextCount * 10 <= sampleCount;
}

bool sdManagerIsConfigPath(const String& path) {
  return sdManagerPathEquals(path, WIFI_CONFIG_PATH) ||
         sdManagerPathEquals(path, PI_CONFIG_PATH);
}

bool sdManagerEditableConfigKey(const String& path, const String& key) {
  String normalizedKey = key;
  normalizedKey.trim();
  normalizedKey.toLowerCase();

  if (sdManagerPathEquals(path, WIFI_CONFIG_PATH)) {
    return normalizedKey == "ssid" || normalizedKey == "password";
  }

  if (sdManagerPathEquals(path, PI_CONFIG_PATH)) {
    return normalizedKey == "mqtt_host" || normalizedKey == "mqtt_port" ||
           normalizedKey == "device_id" || normalizedKey == "command_target" ||
           normalizedKey == "project";
  }

  return false;
}

String sdManagerSizeText(uint32_t sizeBytes) {
  if (sizeBytes < 1024) {
    return String(sizeBytes) + "B";
  }

  return String(sizeBytes / 1024UL) + "K";
}

String sdManagerByteSizeText(uint64_t sizeBytes) {
  constexpr uint64_t KIB = 1024ULL;
  constexpr uint64_t MIB = KIB * 1024ULL;
  constexpr uint64_t GIB = MIB * 1024ULL;

  if (sizeBytes >= GIB) {
    const uint64_t whole = sizeBytes / GIB;
    const uint64_t tenths = (sizeBytes % GIB) * 10ULL / GIB;
    return String(static_cast<unsigned long>(whole)) + "." +
           String(static_cast<unsigned long>(tenths)) + "G";
  }

  if (sizeBytes >= MIB) {
    return String(static_cast<unsigned long>(sizeBytes / MIB)) + "M";
  }

  if (sizeBytes >= KIB) {
    return String(static_cast<unsigned long>(sizeBytes / KIB)) + "K";
  }

  return String(static_cast<unsigned long>(sizeBytes)) + "B";
}

String sdManagerCardTypeText(uint8_t cardType) {
  switch (cardType) {
    case CARD_MMC:
      return "MMC";
    case CARD_SD:
      return "SDSC";
    case CARD_SDHC:
      return "SDHC";
    default:
      return "unknown";
  }
}

void sdManagerResetCardInfo() {
  sdManagerCardType = "none";
  sdManagerCardSizeBytes = 0;
  sdManagerCardUsedBytes = 0;
  sdManagerKnownFolderCount = 0;
}

void sdManagerRefreshCardInfo(uint8_t cardType) {
  sdManagerCardType = sdManagerCardTypeText(cardType);
  sdManagerCardSizeBytes = SD.totalBytes();
  sdManagerCardUsedBytes = SD.usedBytes();
  sdManagerKnownFolderCount = 0;

  for (int i = 0; i < SD_MANAGER_FOLDER_COUNT; ++i) {
    File folder = SD.open(SD_MANAGER_FOLDERS[i].path);
    if (folder) {
      if (folder.isDirectory()) {
        ++sdManagerKnownFolderCount;
      }
      folder.close();
    }
  }
}

bool sdManagerBusyForSdWrite() {
  if (voiceMemoRecording) {
    sdManagerStatus = "Busy: recording.";
    return true;
  }

  if (envLogging) {
    sdManagerStatus = "Busy: env logging.";
    return true;
  }

  if (loraDiagInitialized || loraListening || loraPacketMonitorInitialized ||
      loraPacketMonitorListening || loraRangeInitialized || loraRangeListening ||
      loraRangeTransmitting) {
    sdManagerStatus = "Busy: LoRa active.";
    return true;
  }

  return false;
}

void sdManagerResetEntries() {
  for (int i = 0; i < SD_MANAGER_MAX_ENTRIES; ++i) {
    sdManagerEntries[i] = SdManagerEntry();
  }

  sdManagerEntryCount = 0;
  selectedSdManagerEntryIndex = 0;
  sdManagerEntryScrollOffset = 0;
}

void sdManagerResetConfigLines() {
  for (int i = 0; i < SD_MANAGER_CONFIG_MAX_LINES; ++i) {
    sdManagerConfigLines[i] = SdManagerConfigLine();
  }

  sdManagerConfigLineCount = 0;
  selectedSdManagerConfigIndex = 0;
  sdManagerConfigScrollOffset = 0;
  sdManagerConfigEditingValue = false;
  sdManagerEditInput = "";
}

bool selectedSdManagerEntry(SdManagerEntry& entry) {
  if (sdManagerEntryCount <= 0 || selectedSdManagerEntryIndex < 0 ||
      selectedSdManagerEntryIndex >= sdManagerEntryCount) {
    return false;
  }

  entry = sdManagerEntries[selectedSdManagerEntryIndex];
  return entry.active;
}

void clampSdManagerEntrySelection() {
  if (sdManagerEntryCount <= 0) {
    selectedSdManagerEntryIndex = 0;
    sdManagerEntryScrollOffset = 0;
    return;
  }

  selectedSdManagerEntryIndex =
      constrain(selectedSdManagerEntryIndex, 0, sdManagerEntryCount - 1);

  if (selectedSdManagerEntryIndex < sdManagerEntryScrollOffset) {
    sdManagerEntryScrollOffset = selectedSdManagerEntryIndex;
  } else if (selectedSdManagerEntryIndex >=
             sdManagerEntryScrollOffset + SD_MANAGER_VISIBLE_ROWS) {
    sdManagerEntryScrollOffset =
        selectedSdManagerEntryIndex - SD_MANAGER_VISIBLE_ROWS + 1;
  }

  sdManagerEntryScrollOffset =
      constrain(sdManagerEntryScrollOffset, 0,
                max(0, sdManagerEntryCount - SD_MANAGER_VISIBLE_ROWS));
}

void sdManagerSelectEntryByPath(const String& path) {
  if (path.length() > 0) {
    for (int i = 0; i < sdManagerEntryCount; ++i) {
      if (sdManagerEntries[i].active && sdManagerEntries[i].path == path) {
        selectedSdManagerEntryIndex = i;
        clampSdManagerEntrySelection();
        return;
      }
    }
  }

  clampSdManagerEntrySelection();
}

int sdManagerEditableConfigCount() {
  int count = 0;

  for (int i = 0; i < sdManagerConfigLineCount; ++i) {
    if (sdManagerConfigLines[i].editable) {
      ++count;
    }
  }

  return count;
}

int sdManagerEditableLineIndexAt(int editableIndex) {
  int count = 0;

  for (int i = 0; i < sdManagerConfigLineCount; ++i) {
    if (!sdManagerConfigLines[i].editable) {
      continue;
    }

    if (count == editableIndex) {
      return i;
    }

    ++count;
  }

  return -1;
}

int selectedSdManagerEditableLineIndex() {
  return sdManagerEditableLineIndexAt(selectedSdManagerConfigIndex);
}

void clampSdManagerConfigSelection() {
  const int editableCount = sdManagerEditableConfigCount();
  if (editableCount <= 0) {
    selectedSdManagerConfigIndex = 0;
    sdManagerConfigScrollOffset = 0;
    return;
  }

  if (selectedSdManagerConfigIndex < 0) {
    selectedSdManagerConfigIndex = editableCount - 1;
  } else if (selectedSdManagerConfigIndex >= editableCount) {
    selectedSdManagerConfigIndex = 0;
  }

  if (selectedSdManagerConfigIndex < sdManagerConfigScrollOffset) {
    sdManagerConfigScrollOffset = selectedSdManagerConfigIndex;
  } else if (selectedSdManagerConfigIndex >=
             sdManagerConfigScrollOffset + SD_MANAGER_CONFIG_VISIBLE_ROWS) {
    sdManagerConfigScrollOffset =
        selectedSdManagerConfigIndex - SD_MANAGER_CONFIG_VISIBLE_ROWS + 1;
  }

  sdManagerConfigScrollOffset =
      constrain(sdManagerConfigScrollOffset, 0,
                max(0, editableCount - SD_MANAGER_CONFIG_VISIBLE_ROWS));
}

int sdManagerTextLineCount() {
  if (sdManagerTextBuffer.length() == 0) {
    return 0;
  }

  int count = 1;
  for (int i = 0; i < sdManagerTextBuffer.length(); ++i) {
    if (sdManagerTextBuffer.charAt(i) == '\n' &&
        i < sdManagerTextBuffer.length() - 1) {
      ++count;
    }
  }

  return count;
}

String sdManagerTextLineAt(int targetLine) {
  int currentLine = 0;
  int start = 0;

  for (int i = 0; i <= sdManagerTextBuffer.length(); ++i) {
    if (i == sdManagerTextBuffer.length() ||
        sdManagerTextBuffer.charAt(i) == '\n') {
      if (currentLine == targetLine) {
        return sdManagerTextBuffer.substring(start, i);
      }

      ++currentLine;
      start = i + 1;
    }
  }

  return "";
}

void clampSdManagerTextScroll() {
  sdManagerTextScrollOffset =
      constrain(sdManagerTextScrollOffset, 0,
                max(0, sdManagerTextLineCount() - SD_MANAGER_TEXT_VISIBLE_ROWS));
}

String sdManagerDisplayedValue(const SdManagerConfigLine& line,
                               bool editing) {
  String normalizedKey = line.key;
  normalizedKey.trim();
  normalizedKey.toLowerCase();

  if (!editing && normalizedKey == "password" && line.value.length() > 0) {
    return "********";
  }

  return editing ? sdManagerEditInput : line.value;
}

String sdManagerConfigDisplayText(const SdManagerConfigLine& line,
                                  bool editing) {
  return line.key + "=" + sdManagerDisplayedValue(line, editing);
}

bool sdManagerLoadSmallTextFile(const String& path, uint32_t sizeBytes) {
  sdManagerTextBuffer = "";
  sdManagerTextScrollOffset = 0;

  const bool knownText = sdManagerIsTextExtension(path);
  const bool sniffableText = sdManagerCanSniffTextPath(path);
  if (!knownText && !sniffableText) {
    sdManagerStatus = "Binary view only.";
    return false;
  }

  File file = SD.open(path.c_str(), FILE_READ);
  if (!file) {
    sdManagerStatus = "Open failed.";
    return false;
  }

  if (sniffableText && !sdManagerFileLooksText(file)) {
    file.close();
    sdManagerStatus = "Binary view only.";
    return false;
  }

  if (sizeBytes > SD_MANAGER_TEXT_MAX_BYTES &&
      !sdManagerCanPreviewLargeTextPath(path)) {
    file.close();
    sdManagerStatus = "Large text view only.";
    return false;
  }

  const size_t reserveBytes =
      sizeBytes > SD_MANAGER_TEXT_MAX_BYTES ? SD_MANAGER_TEXT_MAX_BYTES
                                            : sizeBytes;
  sdManagerTextBuffer.reserve(reserveBytes + 1);
  while (file.available() &&
         sdManagerTextBuffer.length() < SD_MANAGER_TEXT_MAX_BYTES) {
    const int value = file.read();
    if (value < 0) {
      break;
    }

    const char c = static_cast<char>(value);
    if (c == '\r') {
      continue;
    }
    if (c == '\n' || sdManagerAllowedValueChar(c)) {
      sdManagerTextBuffer += c;
    } else {
      sdManagerTextBuffer += '.';
    }
  }

  const bool truncated = file.available() || sizeBytes > SD_MANAGER_TEXT_MAX_BYTES;
  file.close();
  if (sdManagerTextBuffer.length() == 0) {
    sdManagerStatus = "Empty text file.";
  } else if (truncated && sdManagerCanPreviewLargeTextPath(path)) {
    sdManagerStatus = "CSV preview first 2K.";
  } else {
    sdManagerStatus = "Text loaded.";
  }
  return true;
}

bool sdManagerOpenSelectedFile() {
  SdManagerEntry entry;
  if (!selectedSdManagerEntry(entry)) {
    sdManagerStatus = "No file selected.";
    renderSdManager();
    return false;
  }

  sdManagerSelectedPath = entry.path;
  sdManagerSelectedName = entry.name;
  sdManagerTextBuffer = "";
  sdManagerTextScrollOffset = 0;

  if (entry.directory) {
    sdManagerStatus = "Folder metadata only.";
    sdManagerView = SdManagerView::FileView;
    renderSdManager();
    return true;
  }

  if (!initSdManagerSd()) {
    sdManagerView = SdManagerView::FileView;
    renderSdManager();
    return false;
  }

  File file = SD.open(entry.path.c_str(), FILE_READ);
  if (!file) {
    sdManagerStatus = "Open failed.";
    sdManagerView = SdManagerView::FileView;
    renderSdManager();
    return false;
  }

  const uint32_t sizeBytes = file.size();
  file.close();
  sdManagerLoadSmallTextFile(entry.path, sizeBytes);
  sdManagerView = SdManagerView::FileView;
  renderSdManager();
  return true;
}

String sdManagerSerializedConfig() {
  String output;

  for (int i = 0; i < sdManagerConfigLineCount; ++i) {
    const SdManagerConfigLine& line = sdManagerConfigLines[i];
    if (line.editable) {
      output += line.key;
      output += "=";
      output += line.value;
    } else {
      output += line.raw;
    }
    output += "\n";
  }

  return output;
}

bool sdManagerWriteFileAtomic(const String& path, const String& content) {
  if (!sdManagerSafeFilePath(path) || sdManagerBusyForSdWrite()) {
    return false;
  }

  if (!initSdManagerSd()) {
    return false;
  }

  const String tempPath = path + ".tmp";
  const String backupPath = path + ".bak";

  if (tempPath.length() > path.length() + SD_MANAGER_TEMP_SUFFIX_EXTRA ||
      backupPath.length() > path.length() + SD_MANAGER_BACKUP_SUFFIX_EXTRA ||
      !sdManagerSafeFilePath(tempPath) || !sdManagerSafeFilePath(backupPath)) {
    sdManagerStatus = "Temp path blocked.";
    return false;
  }

  if (SD.exists(tempPath.c_str())) {
    SD.remove(tempPath.c_str());
  }
  if (SD.exists(backupPath.c_str())) {
    SD.remove(backupPath.c_str());
  }

  File tempFile = SD.open(tempPath.c_str(), FILE_WRITE);
  if (!tempFile) {
    sdManagerStatus = "Temp open failed.";
    return false;
  }

  const size_t written = tempFile.print(content);
  tempFile.flush();
  tempFile.close();

  if (written != content.length()) {
    SD.remove(tempPath.c_str());
    sdManagerStatus = "Temp write failed.";
    return false;
  }

  bool hadOriginal = SD.exists(path.c_str());
  if (hadOriginal && !SD.rename(path.c_str(), backupPath.c_str())) {
    SD.remove(tempPath.c_str());
    sdManagerStatus = "Backup rename failed.";
    return false;
  }

  if (!SD.rename(tempPath.c_str(), path.c_str())) {
    if (hadOriginal) {
      SD.rename(backupPath.c_str(), path.c_str());
    }
    SD.remove(tempPath.c_str());
    sdManagerStatus = "Final rename failed.";
    return false;
  }

  if (hadOriginal) {
    SD.remove(backupPath.c_str());
  }

  sdManagerStatus = "Saved by temp rename.";
  return true;
}

String sdManagerDefaultFileContent(const String& path) {
  if (sdManagerPathEquals(path, WIFI_CONFIG_PATH)) {
    return "ssid=\npassword=\n";
  }

  if (sdManagerPathEquals(path, PI_CONFIG_PATH)) {
    return String("mqtt_host=\n") + "mqtt_port=1883\n" +
           "device_id=scoober-cardputer\n" + "command_target=\n";
  }

  return "";
}

bool sdManagerConfirmPendingPathFromName(const String& extensionToPreserve = "") {
  sdManagerPendingName.trim();

  if (extensionToPreserve.length() > 0 &&
      sdManagerLowerExtension(sdManagerPendingName).length() == 0) {
    const String extendedName = sdManagerPendingName + extensionToPreserve;
    if (extendedName.length() > SD_MANAGER_NAME_MAX_LENGTH) {
      sdManagerStatus = "Name too long.";
      renderSdManager();
      return false;
    }

    sdManagerPendingName = extendedName;
  }

  if (!sdManagerSafeFileName(sdManagerPendingName)) {
    sdManagerStatus = "Bad filename.";
    renderSdManager();
    return false;
  }

  const String path = sdManagerBuildPath(sdManagerCurrentFolder,
                                         sdManagerPendingName);
  if (!sdManagerSafeFilePath(path)) {
    sdManagerStatus = "Path blocked.";
    renderSdManager();
    return false;
  }

  sdManagerPendingPath = path;
  return true;
}

bool sdManagerCreatePendingFile() {
  sdManagerResultMessage = "Create failed.";

  if (!sdManagerConfirmPendingPathFromName() || sdManagerBusyForSdWrite()) {
    return false;
  }

  if (!initSdManagerSd()) {
    return false;
  }

  if (!SD.exists(sdManagerCurrentFolder.c_str()) &&
      !SD.mkdir(sdManagerCurrentFolder.c_str())) {
    sdManagerStatus = "Folder create failed.";
    return false;
  }

  if (SD.exists(sdManagerPendingPath.c_str())) {
    sdManagerStatus = "File exists.";
    return false;
  }

  const bool ok =
      sdManagerWriteFileAtomic(sdManagerPendingPath,
                               sdManagerDefaultFileContent(sdManagerPendingPath));
  if (ok) {
    sdManagerSelectedPath = sdManagerPendingPath;
    sdManagerSelectedName = sdManagerPendingName;
    sdManagerResultMessage = "File created.";
    Serial.printf("SD Manager: created %s\n", sdManagerPendingPath.c_str());
  } else {
    sdManagerResultMessage = "Create failed.";
  }

  if (ok) {
    const String createdPath = sdManagerSelectedPath;
    scanSdManagerFolder();
    sdManagerSelectEntryByPath(createdPath);
    sdManagerStatus = "Created + selected.";
  }
  return ok;
}

bool sdManagerRenamePendingFile() {
  sdManagerResultMessage = "Rename failed.";

  SdManagerEntry entry;
  if (!selectedSdManagerEntry(entry) || entry.directory) {
    sdManagerStatus = "Rename blocked.";
    return false;
  }

  if (!sdManagerConfirmPendingPathFromName(sdManagerLowerExtension(entry.name)) ||
      sdManagerBusyForSdWrite()) {
    return false;
  }

  if (!initSdManagerSd()) {
    return false;
  }

  if (!sdManagerSafeFilePath(entry.path) ||
      !sdManagerSafeFilePath(sdManagerPendingPath)) {
    sdManagerStatus = "Path blocked.";
    return false;
  }

  if (SD.exists(sdManagerPendingPath.c_str())) {
    sdManagerStatus = "File exists.";
    return false;
  }

  const bool ok = SD.rename(entry.path.c_str(), sdManagerPendingPath.c_str());
  if (ok) {
    sdManagerSelectedPath = sdManagerPendingPath;
    sdManagerSelectedName = sdManagerPendingName;
    sdManagerResultMessage = "File renamed.";
    Serial.printf("SD Manager: renamed %s to %s\n", entry.path.c_str(),
                  sdManagerPendingPath.c_str());
  } else {
    sdManagerResultMessage = "Rename failed.";
    sdManagerStatus = "Rename failed.";
  }

  if (ok) {
    const String renamedPath = sdManagerSelectedPath;
    scanSdManagerFolder();
    sdManagerSelectEntryByPath(renamedPath);
    sdManagerStatus = "Renamed + selected.";
  }
  return ok;
}

bool sdManagerDeleteSelectedFile() {
  sdManagerResultMessage = "Delete failed.";

  SdManagerEntry entry;
  const int deletedIndex = selectedSdManagerEntryIndex;
  if (!selectedSdManagerEntry(entry) || entry.directory ||
      sdManagerPendingPath.length() == 0 ||
      !sdManagerSafeFilePath(sdManagerPendingPath) || sdManagerBusyForSdWrite()) {
    sdManagerStatus = "Delete blocked.";
    return false;
  }

  if (!initSdManagerSd()) {
    return false;
  }

  const bool ok = SD.remove(sdManagerPendingPath.c_str());
  if (ok) {
    sdManagerResultMessage = "File deleted.";
    Serial.printf("SD Manager: deleted %s\n", sdManagerPendingPath.c_str());
  } else {
    sdManagerResultMessage = "Delete failed.";
    sdManagerStatus = "Delete failed.";
  }

  sdManagerSelectedPath = "";
  sdManagerSelectedName = "";
  if (ok) {
    scanSdManagerFolder();
    if (sdManagerEntryCount > 0) {
      selectedSdManagerEntryIndex = min(deletedIndex, sdManagerEntryCount - 1);
      clampSdManagerEntrySelection();
    }
    sdManagerStatus = "Deleted.";
  }
  return ok;
}

bool sdManagerLoadConfigEditor() {
  if (!sdManagerIsConfigPath(sdManagerSelectedPath)) {
    sdManagerStatus = "Edit blocked.";
    renderSdManager();
    return false;
  }

  if (!initSdManagerSd()) {
    renderSdManager();
    return false;
  }

  File file = SD.open(sdManagerSelectedPath.c_str(), FILE_READ);
  if (!file) {
    sdManagerStatus = "Config open failed.";
    renderSdManager();
    return false;
  }

  if (file.size() > SD_MANAGER_TEXT_MAX_BYTES) {
    file.close();
    sdManagerStatus = "Config too large.";
    renderSdManager();
    return false;
  }

  sdManagerResetConfigLines();
  bool tooManyLines = false;

  while (file.available()) {
    if (sdManagerConfigLineCount >= SD_MANAGER_CONFIG_MAX_LINES) {
      tooManyLines = true;
      break;
    }

    String line = file.readStringUntil('\n');
    if (line.endsWith("\r")) {
      line.remove(line.length() - 1);
    }

    SdManagerConfigLine& configLine =
        sdManagerConfigLines[sdManagerConfigLineCount];
    configLine.raw = line;

    const int equalsIndex = line.indexOf('=');
    if (equalsIndex >= 0) {
      String key = line.substring(0, equalsIndex);
      String value = line.substring(equalsIndex + 1);
      key.trim();
      value.trim();
      configLine.key = key;
      configLine.value = value;
      configLine.editable =
          key.length() > 0 &&
          sdManagerEditableConfigKey(sdManagerSelectedPath, key);
    }

    ++sdManagerConfigLineCount;
  }

  file.close();

  if (tooManyLines) {
    sdManagerResetConfigLines();
    sdManagerStatus = "Too many lines.";
    renderSdManager();
    return false;
  }

  selectedSdManagerConfigIndex = 0;
  sdManagerConfigScrollOffset = 0;
  sdManagerConfigEditingValue = false;
  sdManagerStatus = "OK edit value.";
  sdManagerView = SdManagerView::ConfigEdit;
  renderSdManager();
  return true;
}

bool sdManagerSaveConfigEditor() {
  const int lineIndex = selectedSdManagerEditableLineIndex();
  if (lineIndex < 0 || lineIndex >= sdManagerConfigLineCount) {
    sdManagerStatus = "No key selected.";
    renderSdManager();
    return false;
  }

  sdManagerConfigLines[lineIndex].value = sdManagerEditInput;
  const bool ok =
      sdManagerWriteFileAtomic(sdManagerSelectedPath, sdManagerSerializedConfig());
  sdManagerConfigEditingValue = false;
  if (ok) {
    sdManagerTextBuffer = sdManagerSerializedConfig();
    Serial.printf("SD Manager: saved config %s\n", sdManagerSelectedPath.c_str());
  }

  renderSdManager();
  return ok;
}

void sdManagerBeginCreate() {
  sdManagerPendingName = "";
  sdManagerPendingPath = "";
  sdManagerStatus = "Type new filename.";
  sdManagerView = SdManagerView::CreateName;
  renderSdManager();
}

void sdManagerBeginConfigTemplateCreate(const char* filename) {
  if (!sdManagerInConfigFolder()) {
    sdManagerStatus = "Templates in /config.";
    renderSdManager();
    return;
  }

  sdManagerPendingName = filename;
  sdManagerPendingPath = "";
  if (!sdManagerConfirmPendingPathFromName()) {
    return;
  }

  if (!initSdManagerSd()) {
    renderSdManager();
    return;
  }

  if (SD.exists(sdManagerPendingPath.c_str())) {
    sdManagerStatus = "File exists.";
    renderSdManager();
    return;
  }

  sdManagerStatus = "Review template.";
  sdManagerView = SdManagerView::CreateConfirm;
  renderSdManager();
}

void sdManagerBeginRename() {
  SdManagerEntry entry;
  if (!selectedSdManagerEntry(entry) || entry.directory) {
    sdManagerStatus = "Rename blocked.";
    renderSdManager();
    return;
  }

  sdManagerPendingName = "";
  sdManagerPendingPath = "";
  sdManagerStatus = "Type new filename.";
  sdManagerView = SdManagerView::RenameName;
  renderSdManager();
}

void sdManagerBeginDelete() {
  SdManagerEntry entry;
  if (!selectedSdManagerEntry(entry) || entry.directory) {
    sdManagerStatus = "Delete blocked.";
    renderSdManager();
    return;
  }

  sdManagerPendingName = entry.name;
  sdManagerPendingPath = entry.path;
  sdManagerView = SdManagerView::DeleteConfirm;
  renderSdManager();
}

void sdManagerBeginConfigValueEdit() {
  const int lineIndex = selectedSdManagerEditableLineIndex();
  if (lineIndex < 0 || lineIndex >= sdManagerConfigLineCount) {
    sdManagerStatus = "No key selected.";
    renderSdManager();
    return;
  }

  sdManagerEditInput = sdManagerConfigLines[lineIndex].value;
  sdManagerConfigEditingValue = true;
  sdManagerStatus = "Edit value.";
  renderSdManager();
}

void sdManagerAppendNameChar(char key) {
  if (sdManagerPendingName.length() >= SD_MANAGER_NAME_MAX_LENGTH ||
      !sdManagerAllowedNameChar(key)) {
    return;
  }

  sdManagerPendingName += key;
  renderSdManager();
}

void sdManagerAppendValueChar(char key) {
  if (sdManagerEditInput.length() >= SD_MANAGER_CONFIG_VALUE_MAX_LENGTH ||
      !sdManagerAllowedValueChar(key)) {
    return;
  }

  sdManagerEditInput += key;
  renderSdManager();
}

void renderSdManagerFolderList() {
  beginContentDraw();
  contentCanvas.printf("Status: %s\n",
                       clippedSdManagerText(sdManagerStatus, 23).c_str());

  for (int i = 0; i < SD_MANAGER_FOLDER_COUNT; ++i) {
    const bool selected = i == selectedSdManagerFolderIndex;
    const int rowY = 24 + i * 16;
    if (selected) {
      contentCanvas.fillRect(4, rowY - 1, contentCanvas.width() - 8, 15,
                             DARKGREEN);
      contentCanvas.setTextColor(WHITE, DARKGREEN);
    } else {
      contentCanvas.setTextColor(WHITE, BLACK);
    }

    contentCanvas.setCursor(8, rowY);
    contentCanvas.printf("%c/%s", selected ? '>' : ' ',
                         sdManagerFolderLabel(i).c_str());
  }

  contentCanvas.setTextColor(WHITE, BLACK);
  contentCanvas.setCursor(8, 96);
  contentCanvas.println("OK open I info");
  contentCanvas.println("R retry SD");
  contentCanvas.println("Back menu");
  commitContentDraw();
}

void renderSdManagerCardInfo() {
  beginContentDraw();
  contentCanvas.println("SD Card Info");

  const uint64_t freeBytes = sdManagerCardSizeBytes > sdManagerCardUsedBytes
                                 ? sdManagerCardSizeBytes - sdManagerCardUsedBytes
                                 : 0;
  const String usedText = sdManagerByteSizeText(sdManagerCardUsedBytes);
  const String totalText = sdManagerByteSizeText(sdManagerCardSizeBytes);
  const String freeText = sdManagerByteSizeText(freeBytes);

  contentCanvas.printf("State: %s\n",
                       sdManagerSdAvailable ? "mounted" : "missing");
  contentCanvas.printf("Type: %s\n",
                       clippedSdManagerText(sdManagerCardType, 18).c_str());
  contentCanvas.printf("Used: %s/%s\n", usedText.c_str(), totalText.c_str());
  contentCanvas.printf("Free: %s\n", freeText.c_str());
  contentCanvas.printf("Folders: %d/%d\n", sdManagerKnownFolderCount,
                       SD_MANAGER_FOLDER_COUNT);
  contentCanvas.println();
  contentCanvas.println("R refresh");
  contentCanvas.println("Back folders");
  commitContentDraw();
}

void renderSdManagerFileList() {
  beginContentDraw();
  contentCanvas.printf("%s %d/%d\n", sdManagerCurrentFolder.c_str(),
                       sdManagerEntryCount > 0 ? selectedSdManagerEntryIndex + 1 : 0,
                       sdManagerEntryCount);
  contentCanvas.println(clippedSdManagerText(sdManagerStatus, 28));

  if (sdManagerEntryCount <= 0) {
    contentCanvas.println();
    contentCanvas.println("No files listed.");
    contentCanvas.println(sdManagerInConfigFolder() ? "N new W/P templates"
                                                    : "N new file");
    contentCanvas.println("Back folders");
    commitContentDraw();
    return;
  }

  const int shown = min(sdManagerEntryCount, SD_MANAGER_VISIBLE_ROWS);
  for (int row = 0; row < shown; ++row) {
    const int entryIndex = sdManagerEntryScrollOffset + row;
    if (entryIndex >= sdManagerEntryCount) {
      break;
    }

    const SdManagerEntry& entry = sdManagerEntries[entryIndex];
    const bool selected = entryIndex == selectedSdManagerEntryIndex;
    const int rowY = 30 + row * 15;
    if (selected) {
      contentCanvas.fillRect(4, rowY - 1, contentCanvas.width() - 8, 14,
                             DARKGREEN);
      contentCanvas.setTextColor(WHITE, DARKGREEN);
    } else {
      contentCanvas.setTextColor(WHITE, BLACK);
    }

    contentCanvas.setCursor(8, rowY);
    contentCanvas.printf("%c%c%-15s %5s", selected ? '>' : ' ',
                         entry.directory ? '/' : ' ',
                         clippedSdManagerText(entry.name, 15).c_str(),
                         entry.directory ? "dir" : sdManagerSizeText(entry.size).c_str());
  }

  contentCanvas.setTextColor(WHITE, BLACK);
  contentCanvas.setCursor(8, 108);
  contentCanvas.println(sdManagerInConfigFolder() ? "OK view N/W/P"
                                                  : "OK view N new");
  contentCanvas.println("Back folders");
  commitContentDraw();
}

void renderSdManagerFileView() {
  beginContentDraw();
  contentCanvas.println(clippedSdManagerText(sdManagerSelectedName, 28));

  SdManagerEntry entry;
  if (!selectedSdManagerEntry(entry)) {
    contentCanvas.println("No selected file.");
    contentCanvas.println("Back files");
    commitContentDraw();
    return;
  }

  contentCanvas.printf("%s %s\n", entry.directory ? "Folder" : "File",
                       entry.directory ? "" : sdManagerSizeText(entry.size).c_str());

  if (sdManagerTextBuffer.length() > 0) {
    if (sdManagerStatus == "CSV preview first 2K.") {
      contentCanvas.println("CSV preview first 2K.");
    }
    clampSdManagerTextScroll();
    for (int i = 0; i < SD_MANAGER_TEXT_VISIBLE_ROWS; ++i) {
      const String line = sdManagerTextLineAt(sdManagerTextScrollOffset + i);
      contentCanvas.println(clippedSdManagerText(line, 29));
    }
  } else {
    contentCanvas.println(clippedSdManagerText(sdManagerStatus, 28));
    if (entry.directory) {
      contentCanvas.println("Subfolders are view only.");
    } else if (sdManagerIsConfigPath(entry.path)) {
      contentCanvas.println("E edit key/value");
    } else if (sdManagerLowerExtension(entry.path) == ".wav") {
      contentCanvas.println("WAV view/delete only.");
    } else {
      contentCanvas.println("Metadata view only.");
    }
  }

  contentCanvas.setTextColor(WHITE, BLACK);
  contentCanvas.setCursor(8, 104);
  if (sdManagerIsConfigPath(entry.path) && !entry.directory) {
    contentCanvas.println("E edit D del R ren");
  } else {
    contentCanvas.println("D delete R rename");
  }
  contentCanvas.println("Back files");
  commitContentDraw();
}

void renderSdManagerConfigEdit() {
  beginContentDraw();
  contentCanvas.println(clippedSdManagerText(sdManagerSelectedName, 28));

  const int editableCount = sdManagerEditableConfigCount();
  if (editableCount <= 0) {
    contentCanvas.println("No editable key/value.");
    contentCanvas.println();
    contentCanvas.println("Back file view");
    commitContentDraw();
    return;
  }

  clampSdManagerConfigSelection();
  const int lineIndex = selectedSdManagerEditableLineIndex();
  if (sdManagerConfigEditingValue && lineIndex >= 0) {
    const SdManagerConfigLine& line = sdManagerConfigLines[lineIndex];
    contentCanvas.printf("%s=\n", clippedSdManagerText(line.key, 24).c_str());
    contentCanvas.printf("> %s\n",
                         clippedSdManagerText(sdManagerDisplayedValue(line, true),
                                              28)
                             .c_str());
    contentCanvas.println();
    contentCanvas.println("OK save temp+rename");
    contentCanvas.println("Back delete/cancel");
    contentCanvas.printf("%d/%d chars\n", sdManagerEditInput.length(),
                         SD_MANAGER_CONFIG_VALUE_MAX_LENGTH);
    commitContentDraw();
    return;
  }

  contentCanvas.printf("Key %d/%d\n", selectedSdManagerConfigIndex + 1,
                       editableCount);
  const int shown = min(editableCount, SD_MANAGER_CONFIG_VISIBLE_ROWS);
  for (int row = 0; row < shown; ++row) {
    const int editableIndex = sdManagerConfigScrollOffset + row;
    const int configLineIndex = sdManagerEditableLineIndexAt(editableIndex);
    if (configLineIndex < 0) {
      break;
    }

    const SdManagerConfigLine& line = sdManagerConfigLines[configLineIndex];
    const bool selected = editableIndex == selectedSdManagerConfigIndex;
    const int rowY = 30 + row * 15;
    if (selected) {
      contentCanvas.fillRect(4, rowY - 1, contentCanvas.width() - 8, 14,
                             DARKGREEN);
      contentCanvas.setTextColor(WHITE, DARKGREEN);
    } else {
      contentCanvas.setTextColor(WHITE, BLACK);
    }

    contentCanvas.setCursor(8, rowY);
    contentCanvas.printf("%c%s", selected ? '>' : ' ',
                         clippedSdManagerText(sdManagerConfigDisplayText(line, false),
                                              28)
                             .c_str());
  }

  contentCanvas.setTextColor(WHITE, BLACK);
  contentCanvas.setCursor(8, 108);
  contentCanvas.println("OK edit value");
  contentCanvas.println("Back file view");
  commitContentDraw();
}

void renderSdManagerNameEntry(const char* title, const char* nextAction) {
  beginContentDraw();
  contentCanvas.println(title);
  contentCanvas.printf("Folder: %s\n", sdManagerCurrentFolder.c_str());
  contentCanvas.printf("> %s\n",
                       sdManagerPendingName.length() > 0
                           ? clippedSdManagerText(sdManagerPendingName, 24).c_str()
                           : "(type name)");
  contentCanvas.println();
  contentCanvas.println(nextAction);
  contentCanvas.println("Back delete/cancel");
  contentCanvas.printf("%d/%d chars\n", sdManagerPendingName.length(),
                       SD_MANAGER_NAME_MAX_LENGTH);
  commitContentDraw();
}

void renderSdManagerConfirm(const char* title, const char* okAction) {
  beginContentDraw();
  contentCanvas.println(title);
  contentCanvas.println();
  contentCanvas.println(clippedSdManagerText(sdManagerPendingName, 28));
  contentCanvas.println();
  contentCanvas.println(okAction);
  contentCanvas.println("Back cancel");
  commitContentDraw();
}

void renderSdManagerResult() {
  beginContentDraw();
  contentCanvas.println(sdManagerResultMessage);
  contentCanvas.println();
  contentCanvas.println(clippedSdManagerText(sdManagerPendingName, 28));
  contentCanvas.println();
  contentCanvas.println(clippedSdManagerText(sdManagerStatus, 28));
  contentCanvas.println("OK file list");
  commitContentDraw();
}

}  // namespace

void showSdManager() {
  sdManagerView = SdManagerView::FolderList;
  sdManagerTextBuffer = "";
  sdManagerSelectedPath = "";
  sdManagerSelectedName = "";
  sdManagerPendingName = "";
  sdManagerPendingPath = "";
  sdManagerConfigEditingValue = false;
  sdManagerStatus = "SD check...";
  initSdManagerSd();
  renderSdManager();
}

void renderSdManager() {
  switch (sdManagerView) {
    case SdManagerView::FolderList:
      renderSdManagerFolderList();
      break;
    case SdManagerView::CardInfo:
      renderSdManagerCardInfo();
      break;
    case SdManagerView::FileList:
      renderSdManagerFileList();
      break;
    case SdManagerView::FileView:
      renderSdManagerFileView();
      break;
    case SdManagerView::ConfigEdit:
      renderSdManagerConfigEdit();
      break;
    case SdManagerView::CreateName:
      renderSdManagerNameEntry("Create file", "OK review create");
      break;
    case SdManagerView::CreateConfirm:
      renderSdManagerConfirm("Create file?", "OK create");
      break;
    case SdManagerView::CreateResult:
      renderSdManagerResult();
      break;
    case SdManagerView::RenameName:
      renderSdManagerNameEntry("Rename file", "OK review rename");
      break;
    case SdManagerView::RenameConfirm:
      renderSdManagerConfirm("Rename file?", "OK rename");
      break;
    case SdManagerView::RenameResult:
      renderSdManagerResult();
      break;
    case SdManagerView::DeleteConfirm:
      renderSdManagerConfirm("Delete file?", "OK delete");
      break;
    case SdManagerView::DeleteResult:
      renderSdManagerResult();
      break;
  }
}

bool initSdManagerSd() {
  if (sdManagerBusyForSdWrite()) {
    sdManagerSdAvailable = false;
    sdManagerResetCardInfo();
    return false;
  }

  prepareSharedSpiForSd();

  if (!SD.begin(SD_SPI_CS_PIN, SPI, SD_SPI_FREQUENCY)) {
    sharedSpiOwner = SHARED_SPI_OWNER_NONE;
    sdManagerSdAvailable = false;
    sdManagerResetCardInfo();
    sdManagerStatus = "SD init failed.";
    Serial.println("SD Manager: SD init failed.");
    return false;
  }

  const uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    sdManagerSdAvailable = false;
    sdManagerResetCardInfo();
    sdManagerStatus = "No SD card.";
    Serial.println("SD Manager: no SD card.");
    return false;
  }

  sdManagerSdAvailable = true;
  sdManagerRefreshCardInfo(cardType);
  if (sdManagerStatus.length() == 0 || sdManagerStatus == "No SD card." ||
      sdManagerStatus == "SD init failed." || sdManagerStatus == "SD check...") {
    sdManagerStatus = "SD ready.";
  }
  return true;
}

void scanSdManagerFolder() {
  sdManagerResetEntries();
  sdManagerTextBuffer = "";

  if (!initSdManagerSd()) {
    return;
  }

  if (!sdManagerIsKnownFolder(sdManagerCurrentFolder) ||
      !sdManagerSafePath(sdManagerCurrentFolder)) {
    sdManagerStatus = "Folder blocked.";
    return;
  }

  if (!SD.exists(sdManagerCurrentFolder.c_str())) {
    sdManagerStatus = "Folder missing. N new";
    return;
  }

  File dir = SD.open(sdManagerCurrentFolder.c_str());
  if (!dir || !dir.isDirectory()) {
    if (dir) {
      dir.close();
    }
    sdManagerStatus = "Open folder failed.";
    return;
  }

  bool limitHit = false;
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;
    }

    if (sdManagerEntryCount >= SD_MANAGER_MAX_ENTRIES) {
      limitHit = true;
      entry.close();
      break;
    }

    String entryName = entry.name();
    const int slashIndex = entryName.lastIndexOf('/');
    String displayName =
        slashIndex >= 0 ? entryName.substring(slashIndex + 1) : entryName;
    if (displayName.startsWith(".")) {
      entry.close();
      continue;
    }

    String fullPath = entryName.startsWith("/")
                          ? entryName
                          : sdManagerBuildPath(sdManagerCurrentFolder, entryName);
    if (!sdManagerSafePath(fullPath)) {
      entry.close();
      continue;
    }

    SdManagerEntry& item = sdManagerEntries[sdManagerEntryCount];
    item.active = true;
    item.directory = entry.isDirectory();
    item.name = displayName;
    item.path = fullPath;
    item.size = entry.size();
    ++sdManagerEntryCount;
    entry.close();
  }

  dir.close();

  if (sdManagerEntryCount == 0) {
    sdManagerStatus = "No files. N new";
  } else if (limitHit) {
    sdManagerStatus = "Showing first files.";
  } else {
    sdManagerStatus = "Folder ready.";
  }
}

void moveSdManagerSelection(int direction) {
  if (direction == 0) {
    return;
  }

  if (sdManagerView == SdManagerView::FolderList) {
    selectedSdManagerFolderIndex += direction;
    if (selectedSdManagerFolderIndex < 0) {
      selectedSdManagerFolderIndex = SD_MANAGER_FOLDER_COUNT - 1;
    } else if (selectedSdManagerFolderIndex >= SD_MANAGER_FOLDER_COUNT) {
      selectedSdManagerFolderIndex = 0;
    }
    renderSdManager();
    return;
  }

  if (sdManagerView == SdManagerView::FileList) {
    if (sdManagerEntryCount <= 0) {
      return;
    }

    selectedSdManagerEntryIndex += direction;
    if (selectedSdManagerEntryIndex < 0) {
      selectedSdManagerEntryIndex = sdManagerEntryCount - 1;
    } else if (selectedSdManagerEntryIndex >= sdManagerEntryCount) {
      selectedSdManagerEntryIndex = 0;
    }

    clampSdManagerEntrySelection();
    renderSdManager();
    return;
  }

  if (sdManagerView == SdManagerView::FileView &&
      sdManagerTextBuffer.length() > 0) {
    sdManagerTextScrollOffset += direction;
    clampSdManagerTextScroll();
    renderSdManager();
    return;
  }

  if (sdManagerView == SdManagerView::ConfigEdit &&
      !sdManagerConfigEditingValue) {
    selectedSdManagerConfigIndex += direction;
    clampSdManagerConfigSelection();
    renderSdManager();
  }
}

void enterSdManagerSelection() {
  if (sdManagerView == SdManagerView::FolderList) {
    if (!initSdManagerSd()) {
      renderSdManager();
      return;
    }

    sdManagerCurrentFolder = sdManagerFolderPath(selectedSdManagerFolderIndex);
    scanSdManagerFolder();
    sdManagerView = SdManagerView::FileList;
    renderSdManager();
    return;
  }

  if (sdManagerView == SdManagerView::CardInfo) {
    sdManagerView = SdManagerView::FolderList;
    renderSdManager();
    return;
  }

  if (sdManagerView == SdManagerView::FileList) {
    sdManagerOpenSelectedFile();
    return;
  }

  if (sdManagerView == SdManagerView::FileView) {
    SdManagerEntry entry;
    if (selectedSdManagerEntry(entry) && sdManagerIsConfigPath(entry.path) &&
        !entry.directory) {
      sdManagerLoadConfigEditor();
    } else {
      sdManagerOpenSelectedFile();
    }
    return;
  }

  if (sdManagerView == SdManagerView::ConfigEdit) {
    if (sdManagerConfigEditingValue) {
      sdManagerSaveConfigEditor();
    } else {
      sdManagerBeginConfigValueEdit();
    }
    return;
  }

  if (sdManagerView == SdManagerView::CreateName) {
    if (sdManagerConfirmPendingPathFromName()) {
      sdManagerView = SdManagerView::CreateConfirm;
      renderSdManager();
    }
    return;
  }

  if (sdManagerView == SdManagerView::CreateConfirm) {
    sdManagerCreatePendingFile();
    sdManagerView = SdManagerView::CreateResult;
    renderSdManager();
    return;
  }

  if (sdManagerView == SdManagerView::RenameName) {
    SdManagerEntry entry;
    const String extension =
        selectedSdManagerEntry(entry) ? sdManagerLowerExtension(entry.name) : "";
    if (sdManagerConfirmPendingPathFromName(extension)) {
      sdManagerView = SdManagerView::RenameConfirm;
      renderSdManager();
    }
    return;
  }

  if (sdManagerView == SdManagerView::RenameConfirm) {
    sdManagerRenamePendingFile();
    sdManagerView = SdManagerView::RenameResult;
    renderSdManager();
    return;
  }

  if (sdManagerView == SdManagerView::DeleteConfirm) {
    sdManagerDeleteSelectedFile();
    sdManagerView = SdManagerView::DeleteResult;
    renderSdManager();
    return;
  }

  if (sdManagerView == SdManagerView::CreateResult ||
      sdManagerView == SdManagerView::RenameResult ||
      sdManagerView == SdManagerView::DeleteResult) {
    sdManagerView = SdManagerView::FileList;
    renderSdManager();
  }
}

bool returnSdManagerView() {
  if (sdManagerView == SdManagerView::FolderList) {
    return false;
  }

  if (sdManagerView == SdManagerView::FileList) {
    sdManagerView = SdManagerView::FolderList;
  } else if (sdManagerView == SdManagerView::CardInfo) {
    sdManagerView = SdManagerView::FolderList;
  } else if (sdManagerView == SdManagerView::FileView) {
    sdManagerView = SdManagerView::FileList;
  } else if (sdManagerView == SdManagerView::ConfigEdit) {
    sdManagerConfigEditingValue = false;
    sdManagerView = SdManagerView::FileView;
  } else if (sdManagerView == SdManagerView::CreateConfirm) {
    sdManagerView = SdManagerView::CreateName;
  } else if (sdManagerView == SdManagerView::RenameConfirm) {
    sdManagerView = SdManagerView::RenameName;
  } else if (sdManagerView == SdManagerView::DeleteConfirm) {
    sdManagerView = SdManagerView::FileView;
  } else if (sdManagerView == SdManagerView::CreateName ||
             sdManagerView == SdManagerView::RenameName ||
             sdManagerView == SdManagerView::CreateResult ||
             sdManagerView == SdManagerView::RenameResult ||
             sdManagerView == SdManagerView::DeleteResult) {
    sdManagerView = SdManagerView::FileList;
  }

  renderSdManager();
  return true;
}

bool handleSdManagerBackKey() {
  if (sdManagerView == SdManagerView::ConfigEdit &&
      sdManagerConfigEditingValue) {
    if (sdManagerEditInput.length() > 0) {
      sdManagerEditInput.remove(sdManagerEditInput.length() - 1);
      renderSdManager();
      return true;
    }

    sdManagerConfigEditingValue = false;
    renderSdManager();
    return true;
  }

  if (sdManagerView == SdManagerView::CreateName ||
      sdManagerView == SdManagerView::RenameName) {
    if (sdManagerPendingName.length() > 0) {
      sdManagerPendingName.remove(sdManagerPendingName.length() - 1);
      renderSdManager();
      return true;
    }
  }

  return returnSdManagerView();
}

void handleSdManagerKey(const Keyboard_Class::KeysState& keys) {
  for (char key : keys.word) {
    if (sdManagerView == SdManagerView::CreateName ||
        sdManagerView == SdManagerView::RenameName) {
      sdManagerAppendNameChar(key);
      continue;
    }

    if (sdManagerView == SdManagerView::ConfigEdit &&
        sdManagerConfigEditingValue) {
      sdManagerAppendValueChar(key);
      continue;
    }

    if (key == ';' || key == ',') {
      moveSdManagerSelection(-1);
    } else if (key == '.' || key == '/') {
      moveSdManagerSelection(1);
    } else if (key == 'r' || key == 'R') {
      if (sdManagerView == SdManagerView::FolderList) {
        initSdManagerSd();
        renderSdManager();
      } else if (sdManagerView == SdManagerView::CardInfo) {
        initSdManagerSd();
        renderSdManager();
      } else if (sdManagerView == SdManagerView::FileList) {
        scanSdManagerFolder();
        renderSdManager();
      } else if (sdManagerView == SdManagerView::FileView) {
        sdManagerBeginRename();
      }
    } else if (key == 'n' || key == 'N') {
      if (sdManagerView == SdManagerView::FileList) {
        sdManagerBeginCreate();
      }
    } else if (key == 'w' || key == 'W') {
      if (sdManagerView == SdManagerView::FileList &&
          sdManagerInConfigFolder()) {
        sdManagerBeginConfigTemplateCreate(SD_MANAGER_WIFI_TEMPLATE_NAME);
      }
    } else if (key == 'p' || key == 'P') {
      if (sdManagerView == SdManagerView::FileList &&
          sdManagerInConfigFolder()) {
        sdManagerBeginConfigTemplateCreate(SD_MANAGER_PI_TEMPLATE_NAME);
      }
    } else if (key == 'i' || key == 'I') {
      if (sdManagerView == SdManagerView::FolderList) {
        initSdManagerSd();
        sdManagerView = SdManagerView::CardInfo;
        renderSdManager();
      }
    } else if (key == 'd' || key == 'D') {
      if (sdManagerView == SdManagerView::FileView) {
        sdManagerBeginDelete();
      }
    } else if (key == 'e' || key == 'E') {
      if (sdManagerView == SdManagerView::FileView) {
        SdManagerEntry entry;
        if (selectedSdManagerEntry(entry) && sdManagerIsConfigPath(entry.path) &&
            !entry.directory) {
          sdManagerLoadConfigEditor();
        }
      } else if (sdManagerView == SdManagerView::ConfigEdit) {
        sdManagerBeginConfigValueEdit();
      }
    }
  }

  if (keys.enter) {
    enterSdManagerSelection();
  }
}
