from firmware_source import firmware_source_text


SOURCE = firmware_source_text()


def test_scoober_menu_structure():
    assert '"Scoober"' in SOURCE
    assert '"Battery"' in SOURCE
    assert '"System"' in SOURCE
    assert '"WiFi Scan"' in SOURCE
    assert '"Saved WiFi"' in SOURCE
    assert '"WiFi Connect"' in SOURCE
    assert '"Voice Memos"' in SOURCE
    assert '"Environment"' in SOURCE
    assert '"RF Scan"' in SOURCE
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
        "OLED Test",
    )
    for label in removed_labels:
        assert label not in SOURCE

    removed_symbols = (
        "showKeyboardTest",
        "showDisplayTest",
        "showSdTest",
        "showEspNowPlaceholder",
    )
    for symbol in removed_symbols:
        assert symbol not in SOURCE

    assert "selectedMenuIndex" in SOURCE
    assert "moveMenuSelection(" in SOURCE
    assert "activateSelectedMenuItem()" in SOURCE
    assert "key == ';'" in SOURCE
    assert "key == '.'" in SOURCE
    assert "keys.enter" in SOURCE


if __name__ == "__main__":
    test_scoober_menu_structure()
    print("Menu structure checks passed.")
