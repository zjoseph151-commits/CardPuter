from pathlib import Path


SOURCE = Path("src/main.cpp").read_text()


def test_wifi_scan_is_scrollable():
    assert "WifiNetwork wifiNetworks[MAX_WIFI_NETWORKS]" in SOURCE
    assert "selectedWifiIndex" in SOURCE
    assert "wifiScrollOffset" in SOURCE
    assert "scanWifiNetworks()" in SOURCE
    assert "renderWifiScan()" in SOURCE
    assert "moveWifiSelection(" in SOURCE
    assert "WiFi.scanDelete()" in SOURCE
    assert "min(wifiNetworkCount, WIFI_VISIBLE_ROWS)" in SOURCE


if __name__ == "__main__":
    test_wifi_scan_is_scrollable()
    print("WiFi scroll checks passed.")

