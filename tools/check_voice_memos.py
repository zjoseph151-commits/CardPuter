from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
README = ROOT / "README.md"

main = firmware_source_text()
readme = README.read_text(encoding="utf-8")

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
    "VOICE_RECORD_SAMPLE_RATE = 16000",
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
    "playSelectedVoiceMemo()",
    "deleteSelectedVoiceMemo()",
    "M5Cardputer.Mic.record",
    "M5Cardputer.Speaker.playRaw",
    "M5Cardputer.Speaker.end()",
    "M5Cardputer.Mic.end()",
]

for token in required_main_tokens:
    assert token in main, f"Missing expected voice memo token: {token}"

assert "Voice Memos" in readme
assert "microSD" in readme
assert "R to start or stop recording" in readme
assert "OK/Enter to play" in readme
assert "D to delete" in readme

print("Voice memo checks passed.")
