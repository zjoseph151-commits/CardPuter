from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
NODE = ROOT / "nodes" / "xiao_sx1262_lora_ack"
SOURCE = (NODE / "src" / "main.cpp").read_text(encoding="utf-8")
PLATFORMIO = (NODE / "platformio.ini").read_text(encoding="utf-8")
README = (NODE / "README.md").read_text(encoding="utf-8")
ROOT_README = (ROOT / "README.md").read_text(encoding="utf-8")
NOTES = (ROOT / "notes.md").read_text(encoding="utf-8")
TODO = (ROOT / "todo.md").read_text(encoding="utf-8")
PLAN = (
    ROOT / "docs" / "superpowers" / "plans" / "2026-09-13-lora-tx-planning.md"
).read_text(encoding="utf-8")


def require_tokens(text, tokens, label):
    for token in tokens:
        assert token in text, f"Missing {label}: {token}"


def test_xiao_sx1262_project_shape():
    require_tokens(
        PLATFORMIO,
        [
            "platform = https://github.com/pioarduino/platform-espressif32.git#54.03.21",
            "board = seeed_xiao_esp32s3",
            "[env:xiao_esp32s3_b2b]",
            "-DXIAO_LORA_NSS_PIN=5",
            "-DXIAO_LORA_NSS_PIN=41",
            "tool-esptoolpy @ platformio/tool-esptoolpy@1.40801.0",
            "pre:../../tools/esptool_compat.py",
            "jgromes/RadioLib",
            "-DARDUINO_USB_MODE=1",
            "-DARDUINO_USB_CDC_ON_BOOT=1",
        ],
        "XIAO SX1262 PlatformIO token",
    )

    require_tokens(
        SOURCE,
        [
            "#include <RadioLib.h>",
            "SX1262 radio = new Module(XIAO_LORA_NSS_PIN, XIAO_LORA_DIO1_PIN,",
            "#define XIAO_LORA_NSS_PIN 5",
            "#define XIAO_LORA_DIO1_PIN 2",
            "#define XIAO_LORA_RST_PIN 3",
            "#define XIAO_LORA_BUSY_PIN 4",
            "#define XIAO_LORA_RF_SWITCH_PIN 1",
            "#define XIAO_LORA_USER_BUTTON_PIN 21",
            "#define XIAO_LORA_SPI_SCK_PIN SCK",
            "#define XIAO_LORA_SPI_MISO_PIN MISO",
            "#define XIAO_LORA_SPI_MOSI_PIN MOSI",
            "XIAO_LORA_FREQUENCY_MHZ = 915.0f",
            "XIAO_LORA_BANDWIDTH_KHZ = 125.0f",
            "XIAO_LORA_SPREADING_FACTOR = 12",
            "XIAO_LORA_CODING_RATE = 5",
            "XIAO_LORA_SYNC_WORD = 0x34",
            "XIAO_LORA_PREAMBLE_LEN = 20",
            "XIAO_LORA_TX_POWER_DBM = 2",
            "XIAO_LORA_TCXO_VOLTAGE = 3.0f",
            "XIAO_LORA_SEND_COOLDOWN_MS = 10000",
            "radio.begin(XIAO_LORA_FREQUENCY_MHZ, XIAO_LORA_BANDWIDTH_KHZ,",
            "radio.setDio2AsRfSwitch(true)",
            "radio.setPacketReceivedAction(setPacketReceivedFlag)",
            "radio.startReceive()",
            "radio.readData(payload)",
            "radio.transmit(txPayload)",
            "prepareReceiveMode()",
            "prepareTransmitMode()",
            "SCBR,PING,1",
            "SCBR,ACK,1,",
            "SCBR,NODE,1,",
            "Commands: p=probe, s=status, r=retry radio",
            "radioStateText(lastRadioState)",
            "RADIOLIB_ERR_CHIP_NOT_FOUND",
            "Retrying SX1262 initialization",
            "Manual probe rate-limited",
            "sendManualProbe()",
        ],
        "XIAO SX1262 source token",
    )


def test_xiao_sx1262_docs():
    require_tokens(
        README,
        [
            "XIAO ESP32-S3 Wio-SX1262 LoRa ACK Node",
            "separate from the active Cardputer firmware",
            "Cardputer",
            "firmware remains RX-only",
            "NSS: GPIO5",
            "DIO1 / IRQ: GPIO2",
            "RST: GPIO3",
            "BUSY: GPIO4",
            "RF_SW: GPIO1",
            "xiao_esp32s3_b2b",
            "NSS 41, DIO1 39, RST 42, BUSY 40, and RF_SW 38",
            "Frequency: 915.0 MHz",
            "Bandwidth: 125 kHz",
            "Spreading factor: SF12",
            "Coding rate: CR 4/5",
            "Sync word: 0x34",
            "TX power: 2 dBm",
            "Manual transmit cooldown: 10 seconds",
            "does not send periodic beacons",
            "state=-2",
            "state_text=chip-not-found",
            "header-connected",
            "type `r`",
            "SCBR,NODE,1,xiao-sx1262-ack",
            "SCBR,PING,1,",
            "SCBR,ACK,1,xiao-sx1262-ack",
            "python -m platformio run -d nodes/xiao_sx1262_lora_ack",
        ],
        "XIAO SX1262 README token",
    )

    for doc_name, text in {
        "README.md": ROOT_README,
        "notes.md": NOTES,
        "todo.md": TODO,
        "LoRa TX plan": PLAN,
    }.items():
        require_tokens(
            text,
            [
                "XIAO ESP32-S3 Wio-SX1262 LoRa ACK Node",
                "nodes/xiao_sx1262_lora_ack",
                "SCBR,NODE,1,xiao-sx1262-ack",
                "SCBR,ACK,1,xiao-sx1262-ack",
                "GPIO5",
                "GPIO2",
                "GPIO1",
                "xiao_esp32s3_b2b",
                "no periodic beacons",
            ],
            f"XIAO SX1262 references in {doc_name}",
        )


if __name__ == "__main__":
    test_xiao_sx1262_project_shape()
    test_xiao_sx1262_docs()
    print("XIAO SX1262 LoRa ACK node checks passed.")
