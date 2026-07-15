#include "app.h"

void handleKeyboard() {
  if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) {
    return;
  }

  Keyboard_Class::KeysState keys = M5Cardputer.Keyboard.keysState();
  printKeyState(keys);

  if (currentScreen != Screen::MainMenu && isMenuBackKey(keys)) {
    if (currentScreen == Screen::WifiSaveConfirm) {
      returnToWifiScan();
    } else if (currentScreen == Screen::SavedWifiDeleteConfirm) {
      setScreen(Screen::SavedWifi);
    } else if (currentScreen == Screen::VoiceMemoDeleteConfirm) {
      setScreen(Screen::VoiceMemos);
    } else if (currentScreen == Screen::EnvironmentLogName) {
      if (envLogNameInput.length() > 0) {
        envLogNameInput.remove(envLogNameInput.length() - 1);
        renderEnvironmentLogName();
      } else {
        setScreen(Screen::Environment);
      }
    } else if (currentScreen == Screen::VoiceMemos && voiceMemoRecording) {
      stopVoiceMemoRecording("Saved");
    } else if (currentScreen == Screen::Environment && envLogging) {
      stopEnvironmentLogging("Log stopped.");
      setScreen(Screen::MainMenu);
    } else {
      setScreen(Screen::MainMenu);
    }
    return;
  }

  if (currentScreen == Screen::MainMenu) {
    for (char key : keys.word) {
      if (key == ';' || key == ',') {
        moveMenuSelection(-1);
      } else if (key == '.' || key == '/') {
        moveMenuSelection(1);
      }
    }

    if (keys.enter) {
      activateSelectedMenuItem();
    }
  } else if (currentScreen == Screen::WifiScan) {
    for (char key : keys.word) {
      if (key == ';' || key == ',') {
        moveWifiSelection(-1);
      } else if (key == '.' || key == '/') {
        moveWifiSelection(1);
      } else if (key == 'r' || key == 'R') {
        scanWifiNetworks();
        renderWifiScan();
      }
    }

    if (keys.enter && wifiNetworkCount > 0) {
      setScreen(Screen::WifiSaveConfirm);
    }
  } else if (currentScreen == Screen::WifiSaveConfirm) {
    if (keys.enter && wifiNetworkCount > 0 && selectedWifiIndex < wifiNetworkCount) {
      wifiSaveResultSsid = wifiNetworks[selectedWifiIndex].ssid;
      wifiSaveResultSsid.trim();
      saveWifiName(wifiNetworks[selectedWifiIndex].ssid);
      setScreen(Screen::WifiSaveResult);
    }
  } else if (currentScreen == Screen::WifiSaveResult) {
    if (keys.enter) {
      returnToWifiScan();
    }
  } else if (currentScreen == Screen::SavedWifi) {
    for (char key : keys.word) {
      if (key == ';' || key == ',') {
        moveSavedWifiSelection(-1);
      } else if (key == '.' || key == '/') {
        moveSavedWifiSelection(1);
      } else if ((key == 'd' || key == 'D') && savedWifiCount > 0) {
        pendingDeleteWifiIndex = selectedSavedWifiIndex;
        pendingDeleteWifiName = savedWifiNames[selectedSavedWifiIndex];
        setScreen(Screen::SavedWifiDeleteConfirm);
      }
    }
  } else if (currentScreen == Screen::SavedWifiDeleteConfirm) {
    if (keys.enter) {
      deleteSavedWifiName(pendingDeleteWifiIndex);
      pendingDeleteWifiIndex = -1;
      pendingDeleteWifiName = "";
      setScreen(Screen::SavedWifiDeleteResult);
    }
  } else if (currentScreen == Screen::SavedWifiDeleteResult) {
    if (keys.enter) {
      setScreen(Screen::SavedWifi);
    }
  } else if (currentScreen == Screen::VoiceMemos) {
    if (voiceMemoRecording) {
      for (char key : keys.word) {
        if (key == 'r' || key == 'R') {
          stopVoiceMemoRecording("Saved");
        }
      }
      return;
    }

    for (char key : keys.word) {
      if (key == ';' || key == ',') {
        moveVoiceMemoSelection(-1);
      } else if (key == '.' || key == '/') {
        moveVoiceMemoSelection(1);
      } else if (key == 'r' || key == 'R') {
        startVoiceMemoRecording();
      } else if ((key == 'd' || key == 'D') && voiceMemoCount > 0) {
        pendingVoiceMemoDeleteName = voiceMemos[selectedVoiceMemoIndex].name;
        pendingVoiceMemoDeletePath = voiceMemos[selectedVoiceMemoIndex].path;
        setScreen(Screen::VoiceMemoDeleteConfirm);
      }
    }

    if (keys.enter && voiceMemoCount > 0) {
      playSelectedVoiceMemo();
    }
  } else if (currentScreen == Screen::VoiceMemoDeleteConfirm) {
    if (keys.enter) {
      deleteSelectedVoiceMemo();
      setScreen(Screen::VoiceMemoDeleteResult);
    }
  } else if (currentScreen == Screen::VoiceMemoDeleteResult) {
    if (keys.enter) {
      pendingVoiceMemoDeleteName = "";
      pendingVoiceMemoDeletePath = "";
      setScreen(Screen::VoiceMemos);
    }
  } else if (currentScreen == Screen::Environment) {
    for (char key : keys.word) {
      if (key == 'l' || key == 'L') {
        if (envLogging) {
          stopEnvironmentLogging("Log stopped.");
        } else {
          setScreen(Screen::EnvironmentLogName);
          return;
        }
        showEnvironment();
      }
    }
  } else if (currentScreen == Screen::EnvironmentLogName) {
    bool changed = false;

    for (char key : keys.word) {
      if (envLogNameInput.length() >= ENV_LOG_NAME_MAX_LENGTH) {
        break;
      }

      if (isAlphaNumeric(key) || key == ' ' || key == '_' || key == '-') {
        envLogNameInput += key;
        changed = true;
      }
    }

    if (keys.enter) {
      startEnvironmentLogging(envLogNameInput);
      setScreen(Screen::Environment);
      return;
    }

    if (changed) {
      renderEnvironmentLogName();
    }
  } else if (currentScreen == Screen::RfScanner) {
    for (char key : keys.word) {
      if (key == 'r' || key == 'R') {
        scanRfChannels();
      } else if (key == ';' || key == ',') {
        moveRfScanSelection(-1);
      } else if (key == '.' || key == '/') {
        moveRfScanSelection(1);
      }
    }

    if (keys.enter) {
      scanRfChannels();
    }
  }
}

bool isMenuBackKey(const Keyboard_Class::KeysState& keys) {
  return keys.del;
}

void printKeyState(const Keyboard_Class::KeysState& keys) {
  Serial.print("Keys:");

  for (char key : keys.word) {
    Serial.print(' ');
    Serial.print(key);
  }

  if (keys.del) {
    Serial.print(" Backspace");
  }
  if (keys.enter) {
    Serial.print(" OK");
  }

  Serial.println();
}
