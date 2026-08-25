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


def test_nrf24_removed_from_cardputer_firmware():
    assert_tokens_absent(
        PLATFORMIO,
        [
            "nrf24/RF24@^1.6.1",
        ],
        "main firmware dependency",
    )

    assert_tokens_absent(
        SOURCE,
        [
            "#include <RF24.h>",
            "RF24",
            "NRF24",
            "nrf24",
            "RfScanner",
            "rfScan",
            "RF_SCAN",
            "showRfScanner",
            "scanRfChannels",
            "renderRfScanner",
            "moveRfScanSelection",
            "powerDownNrf24Radio",
            '"RF Scan"',
        ],
        "active NRF24/RF Scan source token",
    )


def test_nrf24_docs_and_todo_mark_removal():
    require_tokens(
        README,
        [
            "NRF24L01 feature was removed from the active Cardputer firmware",
            "Cap LoRa-1262",
            "RF24 dependency is not used by the main firmware",
        ],
        "README NRF24 removal documentation",
    )

    require_tokens(
        NOTES,
        [
            "NRF24L01 / RF Scan feature was removed from active firmware",
            "Cap LoRa-1262",
            "nodes/xiao_nrf24_oled",
        ],
        "notes NRF24 removal documentation",
    )

    require_tokens(
        TODO,
        [
            "NRF24L01 module feature is removed from the active Cardputer firmware",
            "Cap LoRa-1262",
            "No active NRF24/RF Scan source, menu item, or RF24 dependency should be restored",
        ],
        "TODO NRF24 removal roadmap",
    )

    for doc_name, text in {
        "README.md": README,
        "notes.md": NOTES,
        "todo.md": TODO,
    }.items():
        assert_tokens_absent(
            text,
            [
                "RF Scan is the active NRF24 feature",
                "RF Scan works with the NRF24L01 module",
                "current NRF24 RF Scan also uses EXT SPI-related pins",
                "preserve current NRF24 RF Scan code unless",
            ],
            f"stale active NRF24 wording in {doc_name}",
        )


if __name__ == "__main__":
    test_nrf24_removed_from_cardputer_firmware()
    test_nrf24_docs_and_todo_mark_removal()
    print("NRF24 feature removal checks passed.")
