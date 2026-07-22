#include "app.h"

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

  const bool showStatus = voiceMemoStatus.length() > 0;
  if (showStatus) {
    contentCanvas.println(voiceMemoStatus.substring(0, 28));
  }

  if (voiceMemoCount <= 0) {
    if (showStatus) {
      contentCanvas.println();
    }
    contentCanvas.println("No memos yet.");
    commitContentDraw();
    return;
  }

  contentCanvas.printf("%d memo(s) %d/%d D del\n", voiceMemoCount,
                       selectedVoiceMemoIndex + 1, voiceMemoCount);

  const int listTop = showStatus ? 38 : 26;
  const int shown = min(voiceMemoCount, VOICE_MEMO_VISIBLE_ROWS);
  for (int row = 0; row < shown; ++row) {
    const int memoIndex = voiceMemoScrollOffset + row;
    if (memoIndex >= voiceMemoCount) {
      break;
    }

    const bool selected = memoIndex == selectedVoiceMemoIndex;
    const int rowY = listTop + row * 16;

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
