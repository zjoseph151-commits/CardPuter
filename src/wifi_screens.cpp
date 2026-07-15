#include "app.h"

void showWifiScan() {
  scanWifiNetworks();
  renderWifiScan();
}

void scanWifiNetworks() {
  beginContentDraw();
  contentCanvas.println("Scanning...");
  commitContentDraw();
  Serial.println("Starting WiFi scan...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(100);

  const int networkCount = WiFi.scanNetworks();
  wifiNetworkCount = constrain(networkCount, 0, MAX_WIFI_NETWORKS);
  selectedWifiIndex = 0;
  wifiScrollOffset = 0;

  if (networkCount <= 0) {
    Serial.println("No WiFi networks found.");
  } else {
    Serial.printf("Found %d WiFi network(s)\n", networkCount);

    for (int i = 0; i < wifiNetworkCount; ++i) {
      wifiNetworks[i].ssid = WiFi.SSID(i);
      wifiNetworks[i].rssi = WiFi.RSSI(i);
      wifiNetworks[i].channel = WiFi.channel(i);
      wifiNetworks[i].encryption = WiFi.encryptionType(i);

      Serial.printf("%2d: %s RSSI=%d CH=%d ENC=%d\n", i + 1,
                    wifiNetworks[i].ssid.c_str(), wifiNetworks[i].rssi,
                    wifiNetworks[i].channel, wifiNetworks[i].encryption);
    }
  }

  WiFi.scanDelete();
}

void renderWifiScan() {
  beginContentDraw();

  if (wifiNetworkCount <= 0) {
    contentCanvas.println("No networks found.");
    contentCanvas.println("R rescans.");
    commitContentDraw();
    return;
  }

  contentCanvas.printf("%d/%d OK save R scan\n", selectedWifiIndex + 1, wifiNetworkCount);

  const int shown = min(wifiNetworkCount, WIFI_VISIBLE_ROWS);
  for (int row = 0; row < shown; ++row) {
    const int networkIndex = wifiScrollOffset + row;
    if (networkIndex >= wifiNetworkCount) {
      break;
    }

    const bool selected = networkIndex == selectedWifiIndex;
    const int rowY = 20 + row * 17;

    if (selected) {
      contentCanvas.fillRect(4, rowY - 1, contentCanvas.width() - 8, 16, DARKGREEN);
      contentCanvas.setTextColor(WHITE, DARKGREEN);
    } else {
      contentCanvas.setTextColor(WHITE, BLACK);
    }

    String ssid = wifiNetworks[networkIndex].ssid;
    if (ssid.length() == 0) {
      ssid = "(hidden)";
    }

    contentCanvas.setCursor(8, rowY);
    contentCanvas.printf("%c%-14s %4d", selected ? '>' : ' ', ssid.substring(0, 14).c_str(),
                         wifiNetworks[networkIndex].rssi);
  }

  commitContentDraw();
}

void moveWifiSelection(int direction) {
  if (wifiNetworkCount <= 0) {
    return;
  }

  selectedWifiIndex += direction;
  if (selectedWifiIndex < 0) {
    selectedWifiIndex = wifiNetworkCount - 1;
  } else if (selectedWifiIndex >= wifiNetworkCount) {
    selectedWifiIndex = 0;
  }

  if (selectedWifiIndex < wifiScrollOffset) {
    wifiScrollOffset = selectedWifiIndex;
  } else if (selectedWifiIndex >= wifiScrollOffset + WIFI_VISIBLE_ROWS) {
    wifiScrollOffset = selectedWifiIndex - WIFI_VISIBLE_ROWS + 1;
  }

  renderWifiScan();
}

void renderWifiSaveConfirm() {
  beginContentDraw();

  if (wifiNetworkCount <= 0 || selectedWifiIndex < 0 ||
      selectedWifiIndex >= wifiNetworkCount) {
    contentCanvas.println("No network selected.");
    commitContentDraw();
    return;
  }

  String ssid = wifiNetworks[selectedWifiIndex].ssid;
  ssid.trim();

  if (ssid.length() == 0) {
    contentCanvas.println("Hidden SSID");
    contentCanvas.println("Cannot save name.");
    commitContentDraw();
    return;
  }

  contentCanvas.println("Save this network?");
  contentCanvas.println();
  contentCanvas.printf("%s\n", ssid.substring(0, 22).c_str());
  contentCanvas.println();
  contentCanvas.println("OK save");
  contentCanvas.println("Back cancel");

  if (wifiSaveMessage.length() > 0) {
    contentCanvas.println();
    contentCanvas.print(wifiSaveMessage);
  }

  commitContentDraw();
}

void renderWifiSaveResult() {
  beginContentDraw();

  contentCanvas.println(wifiSaveMessage);
  contentCanvas.println();

  if (wifiSaveResultSsid.length() > 0) {
    contentCanvas.println(wifiSaveResultSsid.substring(0, 22));
    contentCanvas.println();
  }

  contentCanvas.println("OK = WiFi Scan");
  commitContentDraw();
}

void renderSavedWifiList() {
  beginContentDraw();

  if (savedWifiCount <= 0) {
    contentCanvas.println("No saved networks.");
    contentCanvas.println("Save from WiFi Scan.");
    commitContentDraw();
    return;
  }

  contentCanvas.printf("%d saved %d/%d D delete\n", savedWifiCount,
                       selectedSavedWifiIndex + 1, savedWifiCount);

  const int shown = min(savedWifiCount, SAVED_WIFI_VISIBLE_ROWS);
  for (int row = 0; row < shown; ++row) {
    const int savedIndex = savedWifiScrollOffset + row;
    if (savedIndex >= savedWifiCount) {
      break;
    }

    const bool selected = savedIndex == selectedSavedWifiIndex;
    const int rowY = 24 + row * 17;

    if (selected) {
      contentCanvas.fillRect(4, rowY - 1, contentCanvas.width() - 8, 16, DARKGREEN);
      contentCanvas.setTextColor(WHITE, DARKGREEN);
    } else {
      contentCanvas.setTextColor(WHITE, BLACK);
    }

    contentCanvas.setCursor(8, rowY);
    contentCanvas.printf("%c%s", selected ? '>' : ' ',
                         savedWifiNames[savedIndex].substring(0, 22).c_str());
  }

  commitContentDraw();
}

void renderSavedWifiDeleteConfirm() {
  beginContentDraw();

  if (pendingDeleteWifiIndex < 0 || pendingDeleteWifiIndex >= savedWifiCount) {
    contentCanvas.println("No saved network.");
    commitContentDraw();
    return;
  }

  contentCanvas.println("Delete saved network?");
  contentCanvas.println();
  contentCanvas.println(pendingDeleteWifiName.substring(0, 22));
  contentCanvas.println();
  contentCanvas.println("OK delete");
  contentCanvas.println("Back cancel");
  commitContentDraw();
}

void renderSavedWifiDeleteResult() {
  beginContentDraw();

  contentCanvas.println(savedWifiDeleteResultMessage);
  contentCanvas.println();

  if (savedWifiDeleteResultName.length() > 0) {
    contentCanvas.println(savedWifiDeleteResultName.substring(0, 22));
    contentCanvas.println();
  }

  contentCanvas.println("OK = Saved WiFi");
  commitContentDraw();
}

void returnToWifiScan() {
  currentScreen = Screen::WifiScan;
  drawScreenFrame("WiFi Scan");
  renderWifiScan();
}

void loadSavedWifiNames() {
  savedWifiCount = 0;
  selectedSavedWifiIndex = 0;
  savedWifiScrollOffset = 0;

  if (!wifiPrefs.begin("scoober_wifi", true)) {
    Serial.println("Could not open saved WiFi preferences.");
    return;
  }

  const int storedCount =
      constrain(wifiPrefs.getInt("count", 0), 0, MAX_SAVED_WIFI_NAMES);

  for (int i = 0; i < storedCount; ++i) {
    char key[8];
    snprintf(key, sizeof(key), "ssid%02d", i);
    String ssid = wifiPrefs.getString(key, "");
    ssid.trim();

    if (ssid.length() > 0 && savedWifiCount < MAX_SAVED_WIFI_NAMES) {
      savedWifiNames[savedWifiCount++] = ssid;
    }
  }

  wifiPrefs.end();
  Serial.printf("Loaded %d saved WiFi name(s).\n", savedWifiCount);
}

bool persistSavedWifiNames() {
  if (!wifiPrefs.begin("scoober_wifi", false)) {
    Serial.println("Could not write saved WiFi preferences.");
    return false;
  }

  wifiPrefs.putInt("count", savedWifiCount);

  for (int i = 0; i < MAX_SAVED_WIFI_NAMES; ++i) {
    char key[8];
    snprintf(key, sizeof(key), "ssid%02d", i);

    if (i < savedWifiCount) {
      wifiPrefs.putString(key, savedWifiNames[i]);
    } else {
      wifiPrefs.remove(key);
    }
  }

  wifiPrefs.end();
  return true;
}

bool savedWifiNameExists(const String& ssid) {
  for (int i = 0; i < savedWifiCount; ++i) {
    if (savedWifiNames[i] == ssid) {
      return true;
    }
  }

  return false;
}

bool saveWifiName(const String& ssid) {
  String cleanSsid = ssid;
  cleanSsid.trim();

  if (cleanSsid.length() == 0) {
    wifiSaveMessage = "Cannot save hidden SSID.";
    return false;
  }

  if (savedWifiNameExists(cleanSsid)) {
    wifiSaveMessage = "Already saved.";
    return false;
  }

  if (savedWifiCount >= MAX_SAVED_WIFI_NAMES) {
    wifiSaveMessage = "Saved list is full.";
    return false;
  }

  savedWifiNames[savedWifiCount++] = cleanSsid;

  if (!persistSavedWifiNames()) {
    savedWifiCount--;
    wifiSaveMessage = "Save failed.";
    return false;
  }

  selectedSavedWifiIndex = savedWifiCount - 1;
  savedWifiScrollOffset = max(0, savedWifiCount - SAVED_WIFI_VISIBLE_ROWS);
  wifiSaveMessage = "Network saved";
  Serial.printf("Saved WiFi SSID: %s\n", cleanSsid.c_str());
  return true;
}

bool deleteSavedWifiName(int savedIndex) {
  if (savedIndex < 0 || savedIndex >= savedWifiCount) {
    savedWifiDeleteResultName = "";
    savedWifiDeleteResultMessage = "Delete failed";
    return false;
  }

  const String removedName = savedWifiNames[savedIndex];

  for (int i = savedIndex; i < savedWifiCount - 1; ++i) {
    savedWifiNames[i] = savedWifiNames[i + 1];
  }

  savedWifiNames[savedWifiCount - 1] = "";
  savedWifiCount--;

  if (!persistSavedWifiNames()) {
    loadSavedWifiNames();
    savedWifiDeleteResultName = removedName;
    savedWifiDeleteResultMessage = "Delete failed";
    return false;
  }

  if (savedWifiCount <= 0) {
    selectedSavedWifiIndex = 0;
    savedWifiScrollOffset = 0;
  } else {
    selectedSavedWifiIndex = min(selectedSavedWifiIndex, savedWifiCount - 1);
    savedWifiScrollOffset = min(savedWifiScrollOffset,
                                max(0, savedWifiCount - SAVED_WIFI_VISIBLE_ROWS));
  }

  savedWifiDeleteResultName = removedName;
  savedWifiDeleteResultMessage = "Network deleted";
  Serial.printf("Deleted saved WiFi SSID: %s\n", removedName.c_str());
  return true;
}

void moveSavedWifiSelection(int direction) {
  if (savedWifiCount <= 0) {
    return;
  }

  selectedSavedWifiIndex += direction;
  if (selectedSavedWifiIndex < 0) {
    selectedSavedWifiIndex = savedWifiCount - 1;
  } else if (selectedSavedWifiIndex >= savedWifiCount) {
    selectedSavedWifiIndex = 0;
  }

  if (selectedSavedWifiIndex < savedWifiScrollOffset) {
    savedWifiScrollOffset = selectedSavedWifiIndex;
  } else if (selectedSavedWifiIndex >= savedWifiScrollOffset + SAVED_WIFI_VISIBLE_ROWS) {
    savedWifiScrollOffset = selectedSavedWifiIndex - SAVED_WIFI_VISIBLE_ROWS + 1;
  }

  renderSavedWifiList();
}
