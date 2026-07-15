from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
NODE = ROOT / "nodes" / "xiao_nrf24_oled"
SOURCE = (NODE / "src" / "main.cpp").read_text(encoding="utf-8")
PLATFORMIO = (NODE / "platformio.ini").read_text(encoding="utf-8")
README = (NODE / "README.md").read_text(encoding="utf-8")
ROOT_README = (ROOT / "README.md").read_text(encoding="utf-8")


def require_tokens(text, tokens, label):
    for token in tokens:
        assert token in text, f"Missing {label}: {token}"


def test_xiao_project_shape():
    require_tokens(
        PLATFORMIO,
        [
            "board = seeed_xiao_esp32c3",
            "nrf24/RF24@^1.6.1",
            "adafruit/Adafruit SSD1306",
            "adafruit/Adafruit GFX Library",
            "adafruit/Adafruit BusIO",
            "-DARDUINO_USB_CDC_ON_BOOT=1",
        ],
        "XIAO PlatformIO token",
    )

    require_tokens(
        SOURCE,
        [
            "#include <Adafruit_SSD1306.h>",
            "#include <RF24.h>",
            "OLED_SDA_PIN = 6",
            "OLED_SCL_PIN = 7",
            "NRF24_SPI_SCK_PIN = 9",
            "NRF24_SPI_MISO_PIN = 21",
            "NRF24_SPI_MOSI_PIN = 10",
            "NRF24_CE_PIN = 20",
            "NRF24_CSN_PIN = 8",
            "NRF24_RX_PIPE = 1",
            "NRF24_DATA_RATE = RF24_250KBPS",
            "BEACON_INTERVAL_MS = 250",
            "REPLY_DELAY_MS = 200",
            "REPLY_REPEAT_COUNT = 3",
            "NRF24_SHARED_ADDRESS",
            "radio.setAddressWidth(5)",
            "radio.setCRCLength(RF24_CRC_16)",
            "radio.disableDynamicPayloads()",
            "radio.setPALevel(RF24_PA_MIN)",
            "radio.setAutoAck(false)",
            "radio.setRetries(0, 0)",
            "radio.openWritingPipe(NRF24_SHARED_ADDRESS)",
            "radio.openReadingPipe(NRF24_RX_PIPE, NRF24_SHARED_ADDRESS)",
            "radio.closeReadingPipe(0)",
            "radio.available(&pipe)",
            "txFailCount",
            "rxFifoFullCount",
            "fifoStateText",
            "sendBeacon()",
            "XIAO beacon",
            "XIAO ack",
            "TX ok",
            "TX failed",
            "display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS, true, false)",
        ],
        "XIAO firmware token",
    )


def test_xiao_docs():
    require_tokens(
        README,
        [
            "XIAO ESP32-C3 NRF24 OLED Test Node",
            "CE -> XIAO D7 / GPIO20",
            "CSN -> XIAO D8 / GPIO8",
            "SCK -> XIAO D9 / GPIO9",
            "MOSI -> XIAO D10 / GPIO10",
            "MISO -> XIAO D6 / GPIO21",
            "shared address `SCBR1`",
            "python -m platformio run -d nodes/xiao_nrf24_oled",
            "XIAO beacon N",
            "XIAO ack N",
            "RF24_250KBPS",
            "RF24_PA_MIN",
            "RF24 hardware ACK is disabled",
            "TX OK/fail",
            "RX pipe",
        ],
        "XIAO README token",
    )

    require_tokens(
        ROOT_README,
        [
            "nodes/xiao_nrf24_oled",
            "XIAO NRF24 OLED Node",
            "CE -> XIAO D7 / GPIO20",
            "CSN -> XIAO D8 / GPIO8",
            "XIAO beacon N",
            "XIAO ack N",
            "RF24_250KBPS",
            "SCBR1",
            "TX attempts",
            "RX pipe",
        ],
        "root README XIAO token",
    )


if __name__ == "__main__":
    test_xiao_project_shape()
    test_xiao_docs()
    print("XIAO NRF24 node checks passed.")
