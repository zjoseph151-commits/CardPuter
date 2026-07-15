from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
PLATFORMIO = (ROOT / "platformio.ini").read_text(encoding="utf-8")
README = (ROOT / "README.md").read_text(encoding="utf-8")
NOTES = (ROOT / "notes.md").read_text(encoding="utf-8")
TODO = (ROOT / "todo.md").read_text(encoding="utf-8")


def require_tokens(text, tokens, label):
    for token in tokens:
        assert token in text, f"Missing {label}: {token}"


def test_nrf24_dependency_and_firmware_shape():
    require_tokens(
        PLATFORMIO,
        [
            "nrf24/RF24@^1.6.1",
        ],
        "PlatformIO NRF24 dependency",
    )

    require_tokens(
        SOURCE,
        [
            "#include <RF24.h>",
            "NRF24_SPI_SCK_PIN = 40",
            "NRF24_SPI_MISO_PIN = 39",
            "NRF24_SPI_MOSI_PIN = 14",
            "NRF24_CSN_PIN = 5",
            "NRF24_CE_PIN = 4",
            "NRF24_RX_PIPE = 1",
            "NRF24_DATA_RATE = RF24_250KBPS",
            "RF_SCAN_CHANNEL_COUNT = 126",
            "RF_SCAN_SAMPLE_COUNT",
            "RF_SCAN_QUIET_COUNT",
            "RF_SCAN_DWELL_MS",
            "RF24 nrf24Radio",
            "Screen::RfScanner",
            '{"RF Scan", Screen::RfScanner}',
            "showRfScanner()",
            "initNrf24Radio()",
            "powerDownNrf24Radio()",
            "scanRfChannels()",
            "renderRfScanner()",
            "drawRfScanGraph()",
            "updateQuietRfChannels()",
            "isQuietRfChannel(uint8_t channel)",
            "moveRfScanSelection(int direction)",
            "nrf24Radio.begin(&SPI, NRF24_CE_PIN, NRF24_CSN_PIN)",
            "nrf24Radio.isChipConnected()",
            "NRF24_SHARED_ADDRESS",
            "nrf24Radio.setAddressWidth(5)",
            "nrf24Radio.setCRCLength(RF24_CRC_16)",
            "nrf24Radio.disableDynamicPayloads()",
            "nrf24Radio.setAutoAck(false)",
            "nrf24Radio.setRetries(0, 0)",
            "nrf24Radio.openReadingPipe(NRF24_RX_PIPE, NRF24_SHARED_ADDRESS)",
            "nrf24Radio.closeReadingPipe(0)",
            "for (uint8_t channel = 0; channel < RF_SCAN_CHANNEL_COUNT; ++channel)",
            "nrf24Radio.testRPD() || nrf24Radio.testCarrier()",
            "rfScanActivity[channel] = activity",
            "Quiet:",
            "Act:%u/%u",
            "R scan",
            "No radio on SPI.",
        ],
        "NRF24 firmware token",
    )


def test_nrf24_docs_and_todo():
    require_tokens(
        README,
        [
            "### RF Scan",
            "RF24",
            "CE: `G4`",
            "CSN: `G5`",
            "SCK: `G40`",
            "MOSI: `G14`",
            "MISO: `G39`",
            "Press `R`",
            "channels `0-125`",
            "bar graph",
            "quiet channels",
            "nodes/xiao_nrf24_oled",
            "RF24_250KBPS",
            "RF channel scanner",
        ],
        "README RF scan documentation",
    )

    require_tokens(
        NOTES,
        [
            "## RF Scan Notes",
            "CE: G4",
            "CSN: G5",
            "shared SPI",
            "RF24",
            "channels 0-125",
            "bar graph",
            "quiet channels",
            "testRPD",
            "testCarrier",
            "## Retired XIAO Two-Node Findings",
            "nodes/xiao_nrf24_oled",
        ],
        "notes RF scan documentation",
    )

    require_tokens(
        TODO,
        [
            "NRF24L01 Module Integration",
            "RF channel scanner",
            "Sweep channels 0-125",
            "bar graph",
            "quiet channels",
            "XIAO ESP32-C3",
            "retired",
        ],
        "TODO RF scan roadmap",
    )


if __name__ == "__main__":
    test_nrf24_dependency_and_firmware_shape()
    test_nrf24_docs_and_todo()
    print("NRF24 feature checks passed.")
