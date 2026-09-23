from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
NODE = ROOT / "nodes" / "xiao_sx1262_lora_ack"
SOURCE = (NODE / "src" / "main.cpp").read_text(encoding="utf-8")
PLATFORMIO = (NODE / "platformio.ini").read_text(encoding="utf-8")
README = (NODE / "README.md").read_text(encoding="utf-8")


def require(text, *tokens):
    for token in tokens:
        assert token in text, f"Missing: {token}"


def test_node():
    require(PLATFORMIO, "[env:xiao_esp32s3_b2b]", "jgromes/RadioLib",
            "-DXIAO_LORA_NSS_PIN=5", "-DXIAO_LORA_NSS_PIN=41")
    require(SOURCE, "XIAO_LORA_TX_POWER_DBM = 5",
            "XIAO_LORA_USE_REGULATOR_LDO = false", "radio.startReceive()",
            "radio.transmit(txPayload)", "SCBR,MSG,1,", "SCBR,MACK,1,",
            "sendMessage(command.substring(2))", "No ACK for message")
    assert "esp_deep_sleep_start()" not in SOURCE
    require(README, "awake serial test peer", "TX power: 5 dBm",
            "DC-DC", "m <text>", "SCBR,MACK,1", "xiao_esp32s3_b2b")


if __name__ == "__main__":
    test_node()
    print("XIAO SX1262 node checks passed.")
