#include "app.h"

void showVoiceMemos() {
  initVoiceMemoSd();
  scanVoiceMemos();
  renderVoiceMemos();
}

bool initVoiceMemoSd() {
  prepareSharedSpiForSd();

  if (voiceSdInitialized && voiceSdAvailable) {
    return true;
  }

  voiceSdInitialized = true;

  if (!SD.begin(SD_SPI_CS_PIN, SPI, SD_SPI_FREQUENCY)) {
    voiceSdAvailable = false;
    sharedSpiOwner = SHARED_SPI_OWNER_NONE;
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
    contentCanvas.printf("%lu KB peak:%d\n", voiceMemoRecordedBytes / 1024UL,
                         voiceMemoLastPeak);
    contentCanvas.println(voiceMemoMicStatus.length() > 0
                              ? voiceMemoMicStatus.substring(0, 28)
                              : voiceMemoChunkPending ? "Mic sampling"
                                                      : "Mic ready");
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

namespace {

struct VoiceMemoCodecRegister {
  uint8_t reg = 0;
  uint8_t value = 0;
};

struct VoiceMemoMicCandidate {
  m5::input_channel_t channel = m5::input_channel_t::input_only_right;
  const char* label = "R";
};

const char* voiceMemoActiveInputLabel = "?";

const char* voiceMemoMicInputLabel(m5::input_channel_t channel) {
  switch (channel) {
    case m5::input_channel_t::input_only_left:
      return "L";
    case m5::input_channel_t::input_stereo:
      return "S";
    case m5::input_channel_t::input_only_right:
    default:
      return "R";
  }
}

const char* voiceMemoBoardLabel() {
  switch (M5.getBoard()) {
    case m5::board_t::board_M5CardputerADV:
      return "ADV";
    case m5::board_t::board_M5Cardputer:
      return "Card";
    default:
      return "Board?";
  }
}

bool voiceMemoUsesEs8311() {
  return M5.getBoard() == m5::board_t::board_M5CardputerADV;
}

void setVoiceMemoMicStatus(const String& status) {
  voiceMemoMicStatus = status;
  Serial.printf("Voice memos: %s\n", voiceMemoMicStatus.c_str());
}

void logVoiceMemoMicConfig(const char* label) {
  const auto micConfig = M5Cardputer.Mic.config();
  Serial.printf(
      "Voice memos mic %s: board=%d enabled=%d running=%d "
      "data=%d ws=%d bck=%d mck=%d port=%d rate=%lu stereo=%d left=%d "
      "adc=%d mag=%u over=%u\n",
      label, static_cast<int>(M5.getBoard()), M5Cardputer.Mic.isEnabled(),
      M5Cardputer.Mic.isRunning(), micConfig.pin_data_in, micConfig.pin_ws,
      micConfig.pin_bck, micConfig.pin_mck, static_cast<int>(micConfig.i2s_port),
      static_cast<unsigned long>(micConfig.sample_rate), micConfig.stereo,
      micConfig.left_channel, micConfig.use_adc, micConfig.magnification,
      micConfig.over_sampling);
}

void configureVoiceMemoMicInput(m5::input_channel_t channel) {
  auto micConfig = M5Cardputer.Mic.config();
  micConfig.pin_data_in = VOICE_MEMO_ADV_MIC_DATA_PIN;
  micConfig.pin_ws = VOICE_MEMO_ADV_MIC_WS_PIN;
  micConfig.pin_bck =
      voiceMemoUsesEs8311() ? VOICE_MEMO_ADV_MIC_BCK_PIN : I2S_PIN_NO_CHANGE;
  micConfig.pin_mck = I2S_PIN_NO_CHANGE;
  micConfig.i2s_port = I2S_NUM_0;
  micConfig.sample_rate = VOICE_RECORD_SAMPLE_RATE;
  micConfig.input_channel = channel;
  micConfig.use_adc = false;
  micConfig.magnification = 16;
  micConfig.over_sampling = 1;
  micConfig.noise_filter_level = 0;
  M5Cardputer.Mic.config(micConfig);
  logVoiceMemoMicConfig("configured");
}

bool enableVoiceMemoMicCodecIfNeeded() {
  if (!voiceMemoUsesEs8311()) {
    return true;
  }

  const bool i2cReady = M5Cardputer.In_I2C.isEnabled() ||
                        M5Cardputer.In_I2C.begin();
  const bool codecSeen =
      i2cReady && M5Cardputer.In_I2C.scanID(VOICE_MEMO_ES8311_ADDRESS,
                                            VOICE_MEMO_AUDIO_I2C_FREQUENCY);

  Serial.printf(
      "Voice memos codec: InI2C=%d port=%d sda=%d scl=%d ES8311=0x%02X %s\n",
      i2cReady, static_cast<int>(M5Cardputer.In_I2C.getPort()),
      M5Cardputer.In_I2C.getSDA(), M5Cardputer.In_I2C.getSCL(),
      VOICE_MEMO_ES8311_ADDRESS, codecSeen ? "seen" : "missing");

  if (!codecSeen) {
    return false;
  }

  static constexpr VoiceMemoCodecRegister registers[] = {
      {0x00, 0x80},  // Reset / power on.
      {0x01, 0xBA},  // MCLK = BCLK.
      {0x02, 0x18},  // Clock pre-divider.
      {0x0D, 0x01},  // Power up analog circuitry.
      {0x0E, 0x02},  // Enable analog PGA and ADC modulator.
      {0x14, 0x10},  // Select Mic1p-Mic1n, minimum PGA gain.
      {0x17, 0xBF},  // ADC volume at the M5Unified default 0 dB point.
      {0x1C, 0x6A},  // Bypass ADC EQ and enable DC offset cancellation.
  };

  bool ok = true;
  for (const auto& codecRegister : registers) {
    if (!M5Cardputer.In_I2C.writeRegister8(
            VOICE_MEMO_ES8311_ADDRESS, codecRegister.reg, codecRegister.value,
            VOICE_MEMO_AUDIO_I2C_FREQUENCY)) {
      ok = false;
    }
    delay(1);
  }

  Serial.printf("Voice memos codec: mic enable %s.\n", ok ? "ok" : "failed");
  return ok;
}

bool beginVoiceMemoMicInput(m5::input_channel_t channel) {
  const unsigned long idleWaitStartedMs = millis();
  while (M5Cardputer.Mic.isRecording()) {
    if (millis() - idleWaitStartedMs >= VOICE_RECORD_CHUNK_TIMEOUT_MS) {
      setVoiceMemoMicStatus("Mic busy timeout");
      M5Cardputer.Mic.end();
      break;
    }
    M5Cardputer.update();
    delay(1);
  }

  M5Cardputer.Mic.end();
  delay(10);
  configureVoiceMemoMicInput(channel);

  if (!enableVoiceMemoMicCodecIfNeeded()) {
    setVoiceMemoMicStatus(String("Codec miss ") + voiceMemoBoardLabel());
    return false;
  }

  if (!M5Cardputer.Mic.begin() || !M5Cardputer.Mic.isEnabled() ||
      !M5Cardputer.Mic.isRunning()) {
    setVoiceMemoMicStatus(String("Mic begin fail ") +
                          voiceMemoMicInputLabel(channel));
    logVoiceMemoMicConfig("begin-failed");
    return false;
  }

  delay(20);
  if (!enableVoiceMemoMicCodecIfNeeded()) {
    setVoiceMemoMicStatus(String("Codec stop ") + voiceMemoBoardLabel());
    M5Cardputer.Mic.end();
    return false;
  }

  logVoiceMemoMicConfig("started");
  return true;
}

bool voiceMemoChunkReady() {
  if (!voiceMemoChunkPending) {
    return false;
  }

  if (M5Cardputer.Mic.isRecording()) {
    return false;
  }

  return millis() - voiceMemoChunkStartedMs >= VOICE_RECORD_CHUNK_READY_MS;
}

bool voiceMemoChunkTimedOut() {
  return voiceMemoChunkPending &&
         millis() - voiceMemoChunkStartedMs >= VOICE_RECORD_CHUNK_TIMEOUT_MS;
}

bool waitForVoiceMemoChunk() {
  while (voiceMemoChunkPending &&
         (M5Cardputer.Mic.isRecording() ||
          millis() - voiceMemoChunkStartedMs < VOICE_RECORD_CHUNK_READY_MS)) {
    if (voiceMemoChunkTimedOut()) {
      return false;
    }
    M5Cardputer.update();
    delay(1);
  }

  return true;
}

int16_t voiceMemoBufferPeak(size_t sampleCount) {
  int32_t peak = 0;

  for (size_t i = 0; i < sampleCount; ++i) {
    int32_t sample = voiceRecordBuffer[i];
    if (sample < 0) {
      sample = sample == INT16_MIN ? INT16_MAX : -sample;
    }

    if (sample > peak) {
      peak = sample;
    }
  }

  return static_cast<int16_t>(peak);
}

bool writeVoiceMemoBufferedChunk() {
  if (!voiceMemoFile) {
    return false;
  }

  size_t bytesToWrite = sizeof(voiceRecordBuffer);
  const uint32_t remainingBytes = VOICE_RECORD_MAX_BYTES - voiceMemoRecordedBytes;
  if (bytesToWrite > remainingBytes) {
    bytesToWrite = remainingBytes;
  }

  if (bytesToWrite == 0) {
    return true;
  }

  const size_t sampleCount = bytesToWrite / sizeof(int16_t);
  voiceMemoLastPeak = voiceMemoBufferPeak(sampleCount);
  if (voiceMemoLastPeak <= VOICE_RECORD_SILENT_PEAK_THRESHOLD) {
    voiceMemoSilentChunkCount++;
    voiceMemoMicStatus = String("Mic ") + voiceMemoActiveInputLabel +
                         " quiet " + String(voiceMemoLastPeak);
  } else {
    voiceMemoMicStatus = String("Mic ") + voiceMemoActiveInputLabel + " pk " +
                         String(voiceMemoLastPeak);
  }

  const size_t written = voiceMemoFile.write(
      reinterpret_cast<const uint8_t*>(voiceRecordBuffer), bytesToWrite);
  if (written != bytesToWrite) {
    return false;
  }

  voiceMemoRecordedBytes += written;
  return true;
}

bool captureVoiceMemoPeak(uint8_t chunks, int16_t& maxPeak) {
  maxPeak = 0;

  for (uint8_t i = 0; i < chunks; ++i) {
    memset(voiceRecordBuffer, 0, sizeof(voiceRecordBuffer));
    if (!M5Cardputer.Mic.record(voiceRecordBuffer, VOICE_RECORD_CHUNK_SAMPLES,
                                VOICE_RECORD_SAMPLE_RATE)) {
      return false;
    }

    voiceMemoChunkPending = true;
    voiceMemoChunkStartedMs = millis();
    if (!waitForVoiceMemoChunk()) {
      voiceMemoChunkPending = false;
      return false;
    }
    voiceMemoChunkPending = false;
    const int16_t peak =
        voiceMemoBufferPeak(sizeof(voiceRecordBuffer) / sizeof(int16_t));
    if (peak > maxPeak) {
      maxPeak = peak;
    }
    Serial.printf("Voice memos mic %s probe %u/%u peak=%d\n",
                  voiceMemoActiveInputLabel, i + 1, chunks, peak);
  }

  return true;
}

bool selectVoiceMemoMicInput() {
  static constexpr VoiceMemoMicCandidate candidates[] = {
      {m5::input_channel_t::input_only_right, "R"},
      {m5::input_channel_t::input_only_left, "L"},
      {m5::input_channel_t::input_stereo, "S"},
  };

  int bestIndex = -1;
  int16_t bestPeak = -1;

  for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
    voiceMemoActiveInputLabel = candidates[i].label;
    setVoiceMemoMicStatus(String("Probe ") + candidates[i].label + " " +
                          voiceMemoBoardLabel());
    voiceMemoStatus = voiceMemoMicStatus;
    renderVoiceMemos();

    int16_t peak = 0;
    const bool ok = beginVoiceMemoMicInput(candidates[i].channel) &&
                    captureVoiceMemoPeak(VOICE_MEMO_MIC_PROBE_CHUNKS, peak);
    M5Cardputer.Mic.end();
    voiceMemoChunkPending = false;
    Serial.printf("Voice memos mic probe result: %s ok=%d peak=%d\n",
                  candidates[i].label, ok, peak);

    if (ok && (bestIndex < 0 || peak > bestPeak)) {
      bestIndex = static_cast<int>(i);
      bestPeak = peak;
    }
  }

  if (bestIndex < 0) {
    setVoiceMemoMicStatus("Mic probe failed");
    return false;
  }

  voiceMemoActiveInputLabel = candidates[bestIndex].label;
  voiceMemoLastPeak = bestPeak;
  setVoiceMemoMicStatus(String("Mic ") + voiceMemoActiveInputLabel +
                        " probe " + String(bestPeak));
  voiceMemoStatus = voiceMemoMicStatus;
  renderVoiceMemos();

  if (!beginVoiceMemoMicInput(candidates[bestIndex].channel)) {
    return false;
  }

  int16_t warmupPeak = 0;
  if (!captureVoiceMemoPeak(1, warmupPeak)) {
    return false;
  }

  voiceMemoLastPeak = warmupPeak;
  setVoiceMemoMicStatus(String("Mic ") + voiceMemoActiveInputLabel +
                        (warmupPeak <= VOICE_RECORD_SILENT_PEAK_THRESHOLD
                             ? " quiet "
                             : " pk ") +
                        String(warmupPeak));
  return true;
}

bool startVoiceMemoChunk() {
  if (voiceMemoChunkPending) {
    return true;
  }

  memset(voiceRecordBuffer, 0, sizeof(voiceRecordBuffer));
  if (!M5Cardputer.Mic.record(voiceRecordBuffer, VOICE_RECORD_CHUNK_SAMPLES,
                              VOICE_RECORD_SAMPLE_RATE)) {
    return false;
  }

  voiceMemoChunkPending = true;
  voiceMemoChunkStartedMs = millis();
  return true;
}

}  // namespace

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
  voiceMemoSilentChunkCount = 0;
  voiceMemoLastPeak = 0;
  voiceMemoMicStatus = "";
  voiceMemoChunkPending = false;
  voiceMemoChunkStartedMs = 0;
  voiceMemoRecordingStartedMs = millis();
  lastVoiceMemoRenderMs = 0;

  M5Cardputer.Speaker.end();

  if (!selectVoiceMemoMicInput() || !startVoiceMemoChunk()) {
    M5Cardputer.Mic.end();
    voiceMemoFile.close();
    SD.remove(activeVoiceMemoPath.c_str());
    voiceMemoChunkPending = false;
    voiceMemoChunkStartedMs = 0;
    voiceMemoStatus = "Mic read failed.";
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

  if (voiceMemoChunkPending) {
    if (!voiceMemoChunkReady()) {
      if (voiceMemoChunkTimedOut()) {
        voiceMemoChunkPending = false;
        stopVoiceMemoRecording("Mic timeout");
        return;
      }

      if (millis() - lastVoiceMemoRenderMs > 250) {
        lastVoiceMemoRenderMs = millis();
        renderVoiceMemos();
      }
      return;
    }

    voiceMemoChunkPending = false;
    if (!writeVoiceMemoBufferedChunk()) {
      stopVoiceMemoRecording("Write failed");
      return;
    }
  }

  if (voiceMemoRecordedBytes >= VOICE_RECORD_MAX_BYTES) {
    stopVoiceMemoRecording("Saved");
    return;
  }

  if (!startVoiceMemoChunk()) {
    stopVoiceMemoRecording("Mic read failed");
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

  const char* finalMessage = message;

  if (voiceMemoChunkPending) {
    const bool chunkReady = waitForVoiceMemoChunk();
    voiceMemoChunkPending = false;
    if (!chunkReady) {
      finalMessage = "Mic timeout";
    } else if (!writeVoiceMemoBufferedChunk()) {
      finalMessage = "Write failed";
    }
  }

  const unsigned long idleWaitStartedMs = millis();
  while (M5Cardputer.Mic.isRecording() &&
         millis() - idleWaitStartedMs < VOICE_RECORD_CHUNK_TIMEOUT_MS) {
    M5Cardputer.update();
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
    voiceMemoStatus =
        strcmp(finalMessage, "Saved") == 0 ? "No audio saved." : finalMessage;
  } else {
    voiceMemoStatus = String(finalMessage) + ": " + activeVoiceMemoName;
    Serial.printf("Saved voice memo: %s (%lu bytes, peak %d)\n",
                  activeVoiceMemoPath.c_str(), voiceMemoRecordedBytes,
                  voiceMemoLastPeak);
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
  voiceMemoChunkPending = false;
  voiceMemoChunkStartedMs = 0;
  voiceMemoMicStatus = "";
  M5Cardputer.Speaker.end();
  M5Cardputer.Mic.end();
}
