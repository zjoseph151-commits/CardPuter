from firmware_source import firmware_source_text


SOURCE = firmware_source_text()


def test_scoober_menu_structure():
    assert '"Scoober"' in SOURCE
    assert '"Battery"' in SOURCE
    assert '"System"' in SOURCE
    assert '"WiFi Scan"' in SOURCE
    assert '"Saved WiFi"' in SOURCE
    assert '"WiFi Connect"' in SOURCE
    assert '"Pi Monitor"' in SOURCE
    assert '"SD Manager"' in SOURCE
    assert '"Voice Memos"' in SOURCE
    assert '"Environment"' in SOURCE
    assert '"OLED Test"' in SOURCE
    assert '"RTC"' in SOURCE
    assert '"GNSS Dash"' in SOURCE
    assert '"GNSS Sky"' in SOURCE
    assert '"Return Home"' in SOURCE
    assert '"Breadcrumbs"' in SOURCE
    assert '"LoRa Messages"' in SOURCE
    assert '"LoRa Diag"' in SOURCE
    assert '"Level"' in SOURCE

    removed_labels = (
        "Keyboard Test",
        "Display Test",
        "SD Card Test",
        "ESP-NOW RC",
        "ESP-NOW RC Placeholder",
        "Battery/System",
        "Backspace/ESC = menu",
        "Project Launcher",
        "IMU Test",
        "RF Scan",
    )
    for label in removed_labels:
        assert label not in SOURCE

    removed_symbols = (
        "showKeyboardTest",
        "showDisplayTest",
        "showSdTest",
        "showEspNowPlaceholder",
        "showRfScanner",
    )
    for symbol in removed_symbols:
        assert symbol not in SOURCE

    removed_screen_symbols = (
        "Screen::RfScanner",
        "RfScanner",
    )
    for symbol in removed_screen_symbols:
        assert symbol not in SOURCE

    assert "selectedMenuIndex" in SOURCE
    assert "moveMenuSelection(" in SOURCE
    assert "activateSelectedMenuItem()" in SOURCE
    assert "Screen::RtcStatus" in SOURCE
    assert "Screen::SdManager" in SOURCE
    assert "Screen::BreadcrumbLogger" in SOURCE
    assert "Screen::LoraMessages" in SOURCE
    assert "{\"LoRa Messages\", Screen::LoraMessages}" in SOURCE
    assert "Screen::LoraPacketMonitor" not in SOURCE
    assert "Screen::LoraRangeTest" not in SOURCE
    assert "key == ';'" in SOURCE
    assert "key == '.'" in SOURCE
    assert "keys.enter" in SOURCE


if __name__ == "__main__":
    test_scoober_menu_structure()
    print("Menu structure checks passed.")
