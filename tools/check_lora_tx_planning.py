from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
PLAN = ROOT / "docs" / "superpowers" / "plans" / "2026-09-13-lora-tx-planning.md"

SOURCE = firmware_source_text()
RX_ONLY_SOURCE = firmware_source_text({"lora_range_test.cpp"})
RANGE_SOURCE = (ROOT / "src" / "lora_range_test.cpp").read_text(
    encoding="utf-8"
)
README = (ROOT / "README.md").read_text(encoding="utf-8")
TODO = (ROOT / "todo.md").read_text(encoding="utf-8")
NOTES = (ROOT / "notes.md").read_text(encoding="utf-8")
PLAN_TEXT = PLAN.read_text(encoding="utf-8")


def require_tokens(text, tokens, label):
    for token in tokens:
        assert token in text, f"Missing {label}: {token}"


def assert_tokens_absent(text, tokens, label):
    for token in tokens:
        assert token not in text, f"Forbidden {label}: {token}"


def test_lora_tx_plan_doc():
    require_tokens(
        PLAN_TEXT,
        [
            "LoRa TX Planning",
            "2026-09-13",
            "LoRa Range Test is the only Cardputer TX feature",
            "US 902-928 MHz",
            "Antenna must be installed",
            "47 CFR 15.5",
            "47 CFR 15.23",
            "47 CFR 15.247",
            "47 CFR 15.249",
            "M5Stack Cap LoRa-1262",
            "915.0 MHz",
            "125 kHz",
            "SF12",
            "CR 4/5",
            "Sync word: 0x34",
            "2 dBm",
            "Rate limit",
            "startTransmit(...)",
            "LoRa Ping / Range Test",
            "second node",
            "canned ASCII payload",
            "ACK",
            "/tracks/lora-rangeNNN.csv",
        ],
        "LoRa TX planning document token",
    )


def test_lora_tx_planning_is_referenced():
    for doc_name, text in {
        "README.md": README,
        "todo.md": TODO,
        "notes.md": NOTES,
    }.items():
        require_tokens(
            text,
            [
                "LoRa TX planning",
                "2026-09-13-lora-tx-planning.md",
                "LoRa Range Test is the only Cardputer TX feature",
                "LoRa Ping / Range Test",
                "US 902-928 MHz",
                "2 dBm",
                "antenna",
            ],
            f"LoRa TX planning reference in {doc_name}",
        )


def test_lora_range_test_is_the_only_tx_path():
    require_tokens(
        RANGE_SOURCE,
        [
            "Screen::LoraRangeTest",
            "LORA_RANGE_SEND_COOLDOWN_MS",
            "LORA_RANGE_ACK_WINDOW_MS",
            "LORA_RANGE_LOG_HEADER",
            "SCBR,PING,1,",
            "SCBR,ACK,1,",
            "startTransmit(payload)",
            "finishTransmit()",
            "setDio1Action(setLoraRangeDio1Flag)",
            "toggleLoraRangeArm()",
            "sendLoraRangePing()",
            "lora-range%03d.csv",
            "ACK timeout",
            "A arm to create CSV",
            "Rate limit: wait ",
        ],
        "LoRa Range Test TX implementation token",
    )

    require_tokens(
        RX_ONLY_SOURCE,
        [
            "LORA_DIAG_RX_FREQUENCY_MHZ = 915.0f",
            "LORA_DIAG_BANDWIDTH_KHZ = 125.0f",
            "LORA_DIAG_SPREADING_FACTOR = 12",
            "LORA_DIAG_CODING_RATE = 5",
            "LORA_DIAG_SYNC_WORD = 0x34",
            "LORA_DIAG_UNUSED_TX_POWER_DBM = 2",
            "LORA_DIAG_PREAMBLE_LEN = 20",
            "LORA_DIAG_NO_TX_NOTICE",
            "LORA_PACKET_MONITOR_NO_TX_NOTICE",
        ],
        "LoRa RX-only firmware token",
    )
    assert_tokens_absent(
        RX_ONLY_SOURCE,
        [
            "startTransmit(",
            ".transmit(",
            "setPacketSentAction(",
        ],
        "LoRa TX token outside LoRa Range Test",
    )


if __name__ == "__main__":
    test_lora_tx_plan_doc()
    test_lora_tx_planning_is_referenced()
    test_lora_range_test_is_the_only_tx_path()
    print("LoRa TX planning checks passed.")
