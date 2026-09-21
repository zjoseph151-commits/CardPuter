from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
SOURCE = firmware_source_text()
RANGE_SOURCE = (ROOT / "src" / "lora_range_test.cpp").read_text(
    encoding="utf-8"
)
README = (ROOT / "README.md").read_text(encoding="utf-8")
NOTES = (ROOT / "notes.md").read_text(encoding="utf-8")
TODO = (ROOT / "todo.md").read_text(encoding="utf-8")


def require_tokens(text, tokens, label):
    for token in tokens:
        assert token in text, f"Missing {label}: {token}"


def test_lora_range_test_source():
    require_tokens(
        SOURCE,
        [
            "Screen::LoraRangeTest",
            '{"LoRa Range", Screen::LoraRangeTest}',
            "LORA_RANGE_SEND_COOLDOWN_MS = 10000",
            "LORA_RANGE_ACK_WINDOW_MS = 10000",
            "LORA_RANGE_LOG_DIR = \"/tracks\"",
            "showLoraRangeTest()",
            "serviceLoraRangeTest();",
            "stopLoraRangeTest();",
            "toggleLoraRangeArm()",
            "sendLoraRangePing()",
            "clearLoraRangeTest()",
            "buildLoraRangeOledHelpLines",
            "drawScreenFrame(\"LoRa Ping / Range Test\")",
            "key == 'a' || key == 'A'",
            "key == 'p' || key == 'P'",
        ],
        "LoRa Range Test integration token",
    )

    require_tokens(
        RANGE_SOURCE,
        [
            "#include <RadioLib.h>",
            "#include \"utility/PI4IOE5V6408_Class.hpp\"",
            "SX1262 loraRangeRadio = new Module(LORA_NSS_PIN, LORA_IRQ_PIN,",
            "prepareSharedExtSpiForRange()",
            "prepareSharedSpiForSd()",
            "SHARED_SPI_OWNER_LORA",
            "loraRangeIoExpander.digitalWrite(LORA_RF_SWITCH_PIN, true)",
            "loraRangeRadio.begin(LORA_DIAG_RX_FREQUENCY_MHZ",
            "loraRangeRadio.setDio1Action(setLoraRangeDio1Flag)",
            "loraRangeRadio.startReceive()",
            "loraRangeRadio.startTransmit(payload)",
            "loraRangeRadio.finishTransmit()",
            "SCBR,PING,1,",
            "SCBR,ACK,1,",
            "rangeCsvField(packet, 4)",
            "lora-range%03d.csv",
            "LORA_RANGE_LOG_HEADER",
            "startLoraGnssSerial()",
            "stopLoraGnssSerial()",
            "A arm to create CSV",
            "Rate limit: wait ",
            "ACK timeout",
            "loraRangeRadio.sleep()",
        ],
        "LoRa Range Test implementation token",
    )


def test_lora_range_test_docs():
    for doc_name, text in {
        "README.md": README,
        "notes.md": NOTES,
        "todo.md": TODO,
    }.items():
        require_tokens(
            text,
            [
                "LoRa Ping / Range Test",
                "LoRa Range Test is the only Cardputer TX feature",
                "SCBR,PING,1",
                "SCBR,ACK,1",
                "/tracks/lora-rangeNNN.csv",
                "10 seconds",
                "antenna",
            ],
            f"LoRa Range Test documentation in {doc_name}",
        )


if __name__ == "__main__":
    test_lora_range_test_source()
    test_lora_range_test_docs()
    print("LoRa Range Test checks passed.")
