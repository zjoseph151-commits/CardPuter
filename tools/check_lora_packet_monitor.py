from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
SOURCE = firmware_source_text()
RX_ONLY_SOURCE = firmware_source_text({"lora_range_test.cpp"})
PACKET_SOURCE = (ROOT / "src" / "lora_packet_monitor.cpp").read_text(
    encoding="utf-8"
)
README = (ROOT / "README.md").read_text(encoding="utf-8")
NOTES = (ROOT / "notes.md").read_text(encoding="utf-8")
TODO = (ROOT / "todo.md").read_text(encoding="utf-8")


def require_tokens(text, tokens, label):
    for token in tokens:
        assert token in text, f"Missing {label}: {token}"


def assert_tokens_absent(text, tokens, label):
    for token in tokens:
        assert token not in text, f"Forbidden {label}: {token}"


def test_lora_packet_monitor_source():
    require_tokens(
        SOURCE,
        [
            "Screen::LoraPacketMonitor",
            '{"LoRa Packets", Screen::LoraPacketMonitor}',
            "LORA_PACKET_MONITOR_SERVICE_INTERVAL_MS = 100",
            "LORA_PACKET_MONITOR_RENDER_INTERVAL_MS = 1000",
            "LORA_PACKET_MONITOR_PAYLOAD_MAX_CHARS = 64",
            "LORA_PACKET_MONITOR_NO_TX_NOTICE",
            "showLoraPacketMonitor()",
            "renderLoraPacketMonitor()",
            "initLoraPacketMonitor()",
            "serviceLoraPacketMonitor();",
            "stopLoraPacketMonitor();",
            "clearLoraPacketMonitor()",
            "buildLoraPacketMonitorOledLines",
            "drawScreenFrame(\"LoRa Packets (RX only)\")",
            "key == 'c' || key == 'C'",
            "Need matching TX",
        ],
        "LoRa Packet Monitor source token",
    )

    require_tokens(
        PACKET_SOURCE,
        [
            "#include <RadioLib.h>",
            "#include \"utility/PI4IOE5V6408_Class.hpp\"",
            "SX1262 packetMonitorRadio = new Module(LORA_NSS_PIN, LORA_IRQ_PIN,",
            "prepareSharedExtSpiForPacketMonitor()",
            "SHARED_SPI_OWNER_LORA",
            "packetMonitorIoExpander.setDirection(LORA_RF_SWITCH_PIN, true)",
            "packetMonitorIoExpander.setHighImpedance(LORA_RF_SWITCH_PIN, false)",
            "packetMonitorIoExpander.digitalWrite(LORA_RF_SWITCH_PIN, true)",
            "packetMonitorRadio.begin(LORA_DIAG_RX_FREQUENCY_MHZ",
            "packetMonitorRadio.setPacketReceivedAction(setPacketMonitorReceivedFlag)",
            "packetMonitorRadio.startReceive()",
            "packetMonitorRadio.readData(packet)",
            "packetMonitorRadio.getRSSI(false)",
            "packetMonitorRadio.getSNR()",
            "sanitizePacketPayload(packet)",
            "RADIOLIB_ERR_CRC_MISMATCH",
            "hasPacket ? \"PktRSSI\" : \"Noise\"",
            "(need matching TX)",
            "OK/R restart C clear",
            "RX only. No TX.",
            "LoRa not found",
        ],
        "LoRa Packet Monitor implementation token",
    )

    assert_tokens_absent(
        RX_ONLY_SOURCE,
        [
            "startTransmit(",
            ".transmit(",
            "setPacketSentAction(",
            "#include <RF24.h>",
            "Screen::RfScanner",
            "showRfScanner",
        ],
        "active TX or retired RF Scan token",
    )


def test_lora_packet_monitor_docs():
    for doc_name, text in {
        "README.md": README,
        "notes.md": NOTES,
        "todo.md": TODO,
    }.items():
        require_tokens(
            text,
            [
                "LoRa Packet Monitor",
                "LoRa Packets",
                "RX-only packet viewer",
                "packet count",
                "payload preview",
                "channel/noise RSSI",
                "RSSI",
                "SNR",
                "matching LoRa",
                "No transmit",
            ],
            f"LoRa Packet Monitor documentation in {doc_name}",
        )


if __name__ == "__main__":
    test_lora_packet_monitor_source()
    test_lora_packet_monitor_docs()
    print("LoRa Packet Monitor checks passed.")
