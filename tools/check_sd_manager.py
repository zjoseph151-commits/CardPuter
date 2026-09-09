from pathlib import Path

from firmware_source import firmware_source_text


ROOT = Path(__file__).resolve().parents[1]
SOURCE = firmware_source_text()
SD_MANAGER_SOURCE = (ROOT / "src" / "sd_manager.cpp").read_text(
    encoding="utf-8"
)
README = (ROOT / "README.md").read_text(encoding="utf-8")
TODO = (ROOT / "todo.md").read_text(encoding="utf-8")


def require_tokens(text, tokens, label):
    for token in tokens:
        assert token in text, f"Missing {label}: {token}"


def test_sd_manager_firmware_is_present():
    require_tokens(
        SOURCE,
        [
            "Screen::SdManager",
            '"SD Manager"',
            "SD_MANAGER_MAX_ENTRIES",
            "SD_MANAGER_TEXT_MAX_BYTES",
            "SD_MANAGER_CONFIG_MAX_LINES",
            "SdManagerView",
            "SdManagerView::FolderList",
            "SdManagerView::CardInfo",
            "SdManagerView::FileList",
            "SdManagerView::FileView",
            "SdManagerView::ConfigEdit",
            "SdManagerView::CreateConfirm",
            "SdManagerView::RenameConfirm",
            "SdManagerView::DeleteConfirm",
            "showSdManager()",
            "renderSdManager()",
            "renderSdManagerCardInfo()",
            "handleSdManagerKey(",
            "handleSdManagerBackKey()",
            "enterSdManagerSelection()",
            "moveSdManagerSelection(",
            "scanSdManagerFolder()",
            "initSdManagerSd()",
        ],
        "SD Manager firmware",
    )


def test_sd_manager_safety_boundaries():
    require_tokens(
        SD_MANAGER_SOURCE,
        [
            '{"config", "/config"}',
            '{"env", "/env"}',
            '{"memos", "/memos"}',
            '{"tracks", BREADCRUMB_LOG_DIR}',
            "sdManagerSafePath(",
            "sdManagerSafeFilePath(",
            "sdManagerSafeFileName(",
            'path.indexOf("..")',
            "sdManagerPathEquals(",
            "normalizedPath.toLowerCase()",
            "normalizedExpected.toLowerCase()",
            "sdManagerEditableConfigKey(",
            "sdManagerPathEquals(path, WIFI_CONFIG_PATH)",
            "sdManagerPathEquals(path, PI_CONFIG_PATH)",
            'normalizedKey == "ssid"',
            'normalizedKey == "password"',
            'normalizedKey == "mqtt_host"',
            'normalizedKey == "mqtt_port"',
            'normalizedKey == "device_id"',
            'normalizedKey == "command_target"',
            'normalizedKey == "project"',
            "voiceMemoRecording",
            "envLogging",
            "loraDiagInitialized",
            "loraListening",
            "loraPacketMonitorInitialized",
            "loraPacketMonitorListening",
            "prepareSharedSpiForSd()",
            "sdManagerCanSniffTextPath(",
            "sdManagerFileLooksText(",
            "sdManagerCanPreviewLargeTextPath(",
            "sdManagerSelectEntryByPath(",
            "sdManagerBeginConfigTemplateCreate(",
            "sdManagerRefreshCardInfo(",
            "SD.totalBytes()",
            "SD.usedBytes()",
            "sdManagerConfirmPendingPathFromName(const String& extensionToPreserve",
            "sdManagerWriteFileAtomic(",
            'path + ".tmp"',
            'path + ".bak"',
            "tempFile.flush()",
            "tempFile.close()",
            "SD.rename",
            "SD.remove",
            "OK delete",
            "OK create",
            "OK rename",
            "Created + selected.",
            "Renamed + selected.",
            "CSV preview first 2K.",
            "N new W/P templates",
            "OK view N/W/P",
            "SD Card Info",
            "Large text view only.",
            "Binary view only.",
            "WAV view/delete only.",
        ],
        "SD Manager safety behavior",
    )


def test_sd_manager_docs_are_current():
    require_tokens(
        README,
        [
            "SD Manager",
            "browse `/config`, `/env`, `/memos`, and `/tracks`",
            "small text files",
            "key/value editor",
            "temp-file save",
            "create, rename, and delete files only after confirmation",
            "SD card info view",
            "large CSV files show a capped first-page preview",
        ],
        "README SD Manager notes",
    )
    require_tokens(
        TODO,
        [
            "Priority 12: SD Manager / Config Editor",
            "First safe SD Manager milestone added",
            "Priority #12 hardware checklist and polish spot-checks passed on 2026-09-08",
            "Priority #12 polish added on 2026-09-08",
            "browse `/config`, `/env`, `/memos`, and `/tracks`",
            "temp-file saves followed by rename",
            "Guard: `tools/check_sd_manager.py`",
        ],
        "TODO SD Manager notes",
    )


if __name__ == "__main__":
    test_sd_manager_firmware_is_present()
    test_sd_manager_safety_boundaries()
    test_sd_manager_docs_are_current()
    print("SD Manager checks passed.")
