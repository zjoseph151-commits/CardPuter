from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
SOURCE = firmware_source_text()
RX_ONLY_SOURCE = firmware_source_text({"lora_messages.cpp"})
MESSAGES = (ROOT / "src" / "lora_messages.cpp").read_text(encoding="utf-8")
DIAG = (ROOT / "src" / "lora_diag.cpp").read_text(encoding="utf-8")
NODE = (ROOT / "nodes" / "xiao_sx1262_lora_ack" / "src" / "main.cpp").read_text(
    encoding="utf-8"
)


def require(text, *tokens):
    for token in tokens:
        assert token in text, f"Missing: {token}"


def test_menu_and_rx_boundary():
    require(SOURCE, '{"LoRa Diag", Screen::LoraDiag}',
            '{"LoRa Messages", Screen::LoraMessages}',
            'buildLoraDiagOledHelpLines', 'buildLoraMessagesOledHelpLines')
    for token in ("Screen::LoraPacketMonitor", "Screen::LoraRangeTest",
                  "SCBR,PING,1,", "lora-range%03d.csv"):
        assert token not in SOURCE, f"Retired firmware token: {token}"
    for token in ("startTransmit(", ".transmit("):
        assert token not in RX_ONLY_SOURCE, f"TX outside messages: {token}"


def test_merged_diagnostics():
    require(DIAG, 'loraDiagPage == 0', 'loraPacketCount++',
            'loraCrcErrorCount++', 'loraReceiveErrorCount++',
            'loraRadio.getRSSI(false)', 'loraRadio.getSNR()',
            'loraDiagLastPacketLength', 'loraDiagLastPacketMs',
            'loraGnssHasFreshFix()', 'loraGnssSatellitesText()',
            'loraGnssHdopText()', 'loraGnssCoordinateText(',
            'loraGnssTimeText()', 'loraGnssLineCount',
            'clearLoraDiagPackets()', 'changeLoraDiagPage(',
            'loraRadio.clearDio1Action()', 'loraRadio.sleep()')


def test_messages_and_peer():
    require(MESSAGES, 'SCBR,MSG,1,', 'SCBR,MACK,1,',
            'LORA_MESSAGE_TEXT_MAX_CHARS', 'validBody(*body)',
            'sender != LORA_MESSAGE_PEER_ID',
            'target != LORA_MESSAGE_DEVICE_ID',
            'sequence != pendingSequence', 'entry.delivered = false',
            'loraMessageHistory[i].delivered = true',
            'LORA_MESSAGE_ACK_WINDOW_MS', 'LORA_MESSAGE_SEND_COOLDOWN_MS',
            'messageRadio.transmit(payload)', 'messageRadio.sleep()')
    require(NODE, 'SCBR,MSG,1,', 'SCBR,MACK,1,',
            'sendMessage(command.substring(2))', 'Message from Cardputer',
            'XIAO_LORA_TX_POWER_DBM = 5',
            'XIAO_LORA_USE_REGULATOR_LDO = false',
            'XIAO_LORA_MESSAGE_ACK_WINDOW_MS')
    assert 'esp_deep_sleep_start()' not in NODE


if __name__ == "__main__":
    test_menu_and_rx_boundary()
    test_merged_diagnostics()
    test_messages_and_peer()
    print("LoRa diagnostic and messaging checks passed.")
