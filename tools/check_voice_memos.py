from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
README = ROOT / "README.md"
PLATFORMIO = ROOT / "platformio.ini"
ESPTOOL_COMPAT = ROOT / "tools" / "esptool_compat.py"

main = firmware_source_text()
readme = README.read_text(encoding="utf-8")
platformio = PLATFORMIO.read_text(encoding="utf-8")
esptool_compat = ESPTOOL_COMPAT.read_text(encoding="utf-8")

required_main_tokens = [
    "#include <SPI.h>",
    "#include <SD.h>",
    "SD_SPI_SCK_PIN",
    "SD_SPI_MISO_PIN",
    "SD_SPI_MOSI_PIN",
    "SD_SPI_CS_PIN",
    "Screen::VoiceMemos",
    "Screen::VoiceMemoDeleteConfirm",
    "Screen::VoiceMemoDeleteResult",
    '"Voice Memos"',
    'VOICE_MEMO_DIR = "/memos"',
    "VOICE_MEMO_ADV_MIC_DATA_PIN = 46",
    "VOICE_MEMO_ADV_MIC_WS_PIN = 43",
    "VOICE_MEMO_ADV_MIC_BCK_PIN = 41",
    "VOICE_MEMO_ES8311_ADDRESS = 0x18",
    "VOICE_RECORD_SAMPLE_RATE = 16000",
    "VOICE_RECORD_CHUNK_SAMPLES = 240",
    "VOICE_RECORD_CHUNK_READY_MS",
    "VOICE_RECORD_CHUNK_TIMEOUT_MS",
    "VOICE_MEMO_MIC_PROBE_CHUNKS",
    "VOICE_RECORD_SILENT_PEAK_THRESHOLD = 12",
    "VOICE_RECORD_MAX_SECONDS = 30",
    "struct WavHeader",
    "initVoiceMemoSd()",
    "scanVoiceMemos()",
    "findNextVoiceMemoPath(",
    "renderVoiceMemos()",
    "startVoiceMemoRecording()",
    "serviceVoiceMemoRecording()",
    "stopVoiceMemoRecording(",
    "writeWavHeader(",
    "voiceMemoChunkPending",
    "voiceMemoMicStatus",
    "voiceMemoChunkReady()",
    "voiceMemoChunkTimedOut()",
    "waitForVoiceMemoChunk()",
    "voiceMemoBufferPeak(",
    "configureVoiceMemoMicInput(",
    "enableVoiceMemoMicCodecIfNeeded()",
    "beginVoiceMemoMicInput(",
    "captureVoiceMemoPeak(",
    "selectVoiceMemoMicInput()",
    "writeVoiceMemoBufferedChunk()",
    "startVoiceMemoChunk()",
    "playSelectedVoiceMemo()",
    "deleteSelectedVoiceMemo()",
    "M5Cardputer.Mic.record",
    "M5Cardputer.Mic.isRecording()",
    "m5::input_channel_t::input_only_right",
    "m5::input_channel_t::input_only_left",
    "m5::input_channel_t::input_stereo",
    "I2S_PIN_NO_CHANGE",
    "M5Cardputer.In_I2C.scanID",
    "M5Cardputer.In_I2C.writeRegister8",
    "M5Cardputer.Speaker.playRaw",
    "M5Cardputer.Speaker.end()",
    "M5Cardputer.Mic.end()",
    "quiet",
    "Probe ",
    "Mic sampling",
    "peak:",
]

for token in required_main_tokens:
    assert token in main, f"Missing expected voice memo token: {token}"

assert "Voice Memos" in readme
assert "microSD" in readme
assert "R to start or stop recording" in readme
assert "OK/Enter to play" in readme
assert "D to delete" in readme
assert "M5Unified Mic.record queues capture asynchronously" in readme
assert "ES8311" in readme
assert "right/left/stereo" in readme
assert "ESP-IDF 5.4" in readme
assert "IDF 5.5" in readme
assert "platform-espressif32.git#54.03.21" in platformio
assert "platformio/tool-esptoolpy@1.40801.0" in platformio
assert "pre:tools/esptool_compat.py" in platformio
assert "post:tools/esptool_compat.py" in platformio
assert "ESPTOOL_COMPAT_ORIGINAL_VERBOSE_ACTION" in esptool_compat
assert '"--flash-mode", "--flash_mode"' in esptool_compat
assert '"write-flash", "write_flash"' in esptool_compat
assert '"default-reset", "default_reset"' in esptool_compat
assert '"hard-reset", "hard_reset"' in esptool_compat
assert "-DARDUINO_USB_CDC_ON_BOOT=1" in platformio
assert "-DARDUINO_USB_MODE=1" in platformio

print("Voice memo checks passed.")
