from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
SOURCE = firmware_source_text()
BREADCRUMB_SOURCE = (ROOT / "src" / "breadcrumb_logger.cpp").read_text(
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


def test_breadcrumb_logger_source():
    require_tokens(
        SOURCE,
        [
            "Screen::BreadcrumbLogger",
            '{"Breadcrumbs", Screen::BreadcrumbLogger}',
            "BREADCRUMB_LOGGER_SERVICE_INTERVAL_MS = 200",
            "BREADCRUMB_LOGGER_RENDER_INTERVAL_MS = 1000",
            "BREADCRUMB_LOG_SAMPLE_INTERVAL_MS = 5000",
            'BREADCRUMB_LOG_DIR = "/tracks"',
            "BREADCRUMB_LOG_HEADER",
            "showBreadcrumbLogger()",
            "renderBreadcrumbLogger()",
            "initBreadcrumbLogger()",
            "serviceBreadcrumbLogger();",
            "stopBreadcrumbLogger();",
            "toggleBreadcrumbLogging()",
            "appendBreadcrumbLogSample()",
            "buildBreadcrumbLoggerOledLines",
            "drawScreenFrame(\"Breadcrumb Logger\")",
            "key == 's' || key == 'S'",
            "startLoraGnssSerial()",
            "serviceLoraGnssSerial()",
            "stopLoraGnssSerial()",
            "prepareSharedSpiForSd()",
            "SD.begin",
            "SD.open",
            "breadcrumbLogFile.flush()",
            "loraGnssHasFreshFix()",
            "No LoRa radio",
        ],
        "Breadcrumb Logger source token",
    )

    require_tokens(
        BREADCRUMB_SOURCE,
        [
            "track%03d.csv",
            "Cannot make /tracks.",
            "Logging wait fix.",
            "Waiting fresh fix.",
            "Sample saved.",
            "S start CSV log",
            "S stop CSV log",
            "OK/R reset",
        ],
        "Breadcrumb Logger implementation token",
    )

    assert_tokens_absent(
        BREADCRUMB_SOURCE,
        [
            "#include <RadioLib.h>",
            "SX1262",
            "loraRadio.begin",
            "startReceive(",
            "readData(",
            "startTransmit(",
            ".transmit(",
            "setPacketSentAction(",
        ],
        "LoRa radio/TX token in Breadcrumb Logger",
    )


def test_breadcrumb_logger_docs():
    for doc_name, text in {
        "README.md": README,
        "notes.md": NOTES,
        "todo.md": TODO,
    }.items():
        require_tokens(
            text,
            [
                "Breadcrumb Logger",
                "/tracks/trackNNN.csv",
                "BREADCRUMB_LOG_SAMPLE_INTERVAL_MS",
                "CSV",
                "fresh GNSS fix",
                "No transmit",
            ],
            f"Breadcrumb Logger documentation in {doc_name}",
        )


if __name__ == "__main__":
    test_breadcrumb_logger_source()
    test_breadcrumb_logger_docs()
    print("Breadcrumb Logger checks passed.")
