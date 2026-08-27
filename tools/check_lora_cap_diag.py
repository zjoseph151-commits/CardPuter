from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
SOURCE = firmware_source_text()
PLATFORMIO = (ROOT / "platformio.ini").read_text(encoding="utf-8")
README = (ROOT / "README.md").read_text(encoding="utf-8")
NOTES = (ROOT / "notes.md").read_text(encoding="utf-8")
TODO = (ROOT / "todo.md").read_text(encoding="utf-8")


def require_tokens(text, tokens, label):
    for token in tokens:
        assert token in text, f"Missing {label}: {token}"


def assert_tokens_absent(text, tokens, label):
    for token in tokens:
        assert token not in text, f"Forbidden {label}: {token}"


def test_lora_cap_diagnostics_source():
    assert "jgromes/RadioLib" in PLATFORMIO, "RadioLib dependency is required"
    assert "https://github.com/m5stack/TinyGPSPlus.git" in PLATFORMIO, (
        "M5Stack TinyGPSPlus dependency is required"
    )
    assert "nrf24/RF24" not in PLATFORMIO, "RF24 must stay out of main firmware"

    require_tokens(
        SOURCE,
        [
            "#include <RadioLib.h>",
            "#include <TinyGPSPlus.h>",
            "#include \"utility/PI4IOE5V6408_Class.hpp\"",
            "Screen::LoraDiag",
            "{\"LoRa Diag\", Screen::LoraDiag}",
            "SX1262 loraRadio = new Module(LORA_NSS_PIN, LORA_IRQ_PIN, LORA_RST_PIN,",
            "HardwareSerial loraGnssSerial(1)",
            "TinyGPSPlus loraGps",
            "loraGps = TinyGPSPlus()",
            "LORA_RST_PIN = 3",
            "LORA_IRQ_PIN = 4",
            "LORA_NSS_PIN = 5",
            "LORA_BUSY_PIN = 6",
            "LORA_SPI_SCK_PIN = 40",
            "LORA_SPI_MOSI_PIN = 14",
            "LORA_SPI_MISO_PIN = 39",
            "LORA_GNSS_TX_PIN = 13",
            "LORA_GNSS_RX_PIN = 15",
            "LORA_GNSS_BAUD = 115200",
            "LORA_IO_EXPANDER_ADDRESS = 0x43",
            "LORA_RF_SWITCH_PIN = 0",
            "loraIoExpander.setDirection(LORA_RF_SWITCH_PIN, true)",
            "loraIoExpander.setHighImpedance(LORA_RF_SWITCH_PIN, false)",
            "loraIoExpander.digitalWrite(LORA_RF_SWITCH_PIN, true)",
            "loraRadio.setPacketReceivedAction(setLoraPacketReceivedFlag)",
            "loraRadio.startReceive()",
            "loraRadio.readData(packet)",
            "loraRadio.getRSSI(false)",
            "--pkt",
            "loraGps.encode(ch)",
            "loraGps.location.isValid()",
            "loraGps.location.lat()",
            "loraGps.location.lng()",
            "loraGps.satellites.value()",
            "loraGps.hdop.hdop()",
            "loraGps.time.hour()",
            "loraGps.date.year()",
            "loraGnssHasFreshFix()",
            "LORA_GNSS_FIX_STALE_MS = 5000",
            "GNSS:%s S:%s HD:%s",
            "SERIAL_8N1",
            "LORA_DIAG_NO_TX_NOTICE",
            "RX only. No TX.",
            "deselectSharedSpiDevices()",
            "prepareSharedSpiForSd()",
            "SHARED_SPI_OWNER_SD",
            "SHARED_SPI_OWNER_LORA",
            "sharedSpiOwner",
            "sharedSpiOwner = SHARED_SPI_OWNER_NONE",
            "SPI.end()",
            "LoRa not found",
            "buildLoraDiagOledLines",
            "serviceLoraDiagnostics();",
            "stopLoraDiagnostics();",
        ],
        "LoRa diagnostics source token",
    )

    assert_tokens_absent(
        SOURCE,
        [
            "startTransmit(",
            ".transmit(",
            "setPacketSentAction(",
            "#include <RF24.h>",
            "Screen::RfScanner",
            "showRfScanner",
            "rfScan",
        ],
        "active TX or retired RF Scan token",
    )


def test_lora_cap_diagnostics_docs():
    for doc_name, text in {
        "README.md": README,
        "notes.md": NOTES,
        "todo.md": TODO,
    }.items():
        require_tokens(
            text,
            [
                "Cap LoRa-1262",
                "SX1262",
                "ATGM336H",
                "RadioLib",
                "TinyGPSPlus",
                "GNSS parser",
                "LoRa not found",
                "No transmit",
                "G5 NSS",
                "G4 IRQ",
                "G3 RST",
                "G6 BUSY",
                "G40 SCK",
                "G14 MOSI",
                "G39 MISO",
                "G15 GPS-TX",
                "G13 GPS-RX",
                "PI4IOE5V6408",
                "P0",
            ],
            f"LoRa diagnostics documentation in {doc_name}",
        )


if __name__ == "__main__":
    test_lora_cap_diagnostics_source()
    test_lora_cap_diagnostics_docs()
    print("LoRa Cap diagnostics checks passed.")
