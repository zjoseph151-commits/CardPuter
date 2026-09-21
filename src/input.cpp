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
    } else if (currentScreen == Screen::SdManager &&
               handleSdManagerBackKey()) {
      return;
    } else if (currentScreen == Screen::PiMonitor &&
               returnPiMonitorProjectList()) {
      return;
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
  } else if (currentScreen == Screen::WifiConnect) {
    for (char key : keys.word) {
      if (key == 'd' || key == 'D') {
        disconnectWifi();
      }
    }

    if (keys.enter) {
      connectWifiFromConfig();
      renderWifiConnect();
    }
  } else if (currentScreen == Screen::PiMonitor) {
    for (char key : keys.word) {
      if (key == ';' || key == ',') {
        movePiMonitorSelection(-1);
      } else if (key == '.' || key == '/') {
        movePiMonitorSelection(1);
      } else if (key == 'r' || key == 'R') {
        clearPiMonitorDevices();
        connectPiMonitorMqtt();
        renderPiMonitor();
      } else if (key == 'd' || key == 'D') {
        disconnectPiMonitorMqtt();
      } else if (key == 't' || key == 'T') {
        cyclePiMonitorCommandTarget();
      } else if (key == 'c' || key == 'C') {
        publishPiMonitorReadNowCommand();
      } else if (key == 'i' || key == 'I') {
        cyclePiMonitorCommandOption(1);
      } else if (key == 's' || key == 'S') {
        advancePiMonitorOledMessagePage();
      }
    }

    if (keys.enter) {
      enterPiMonitorSelection();
    }
  } else if (currentScreen == Screen::SdManager) {
    handleSdManagerKey(keys);
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
  } else if (currentScreen == Screen::OledTest) {
    for (char key : keys.word) {
      if (key == 'r' || key == 'R') {
        oledInitialized = false;
        oledOnline = false;
        oledActiveAddress = 0;
        oledActiveBusFrequency = 0;
        oledScanSummary = "Retrying...";
        renderOledTest();
      }
    }

    if (keys.enter) {
      oledInitialized = false;
      oledOnline = false;
      oledActiveAddress = 0;
      oledActiveBusFrequency = 0;
      oledScanSummary = "Retrying...";
      renderOledTest();
    }
  } else if (currentScreen == Screen::RtcStatus) {
    bool retry = keys.enter;
    bool setClock = false;
    bool setFromNtp = false;

    for (char key : keys.word) {
      if (key == 's' || key == 'S') {
        setClock = true;
      } else if (key == 'n' || key == 'N') {
        setFromNtp = true;
      } else if (key == 'r' || key == 'R') {
        retry = true;
      }
    }

    if (setFromNtp) {
      rtcStatus = "NTP sync...";
      renderRtcStatus();
      renderOledStatusDashboard();
      setRtcFromNtp();
      renderRtcStatus();
      renderOledStatusDashboard();
    } else if (setClock) {
      setRtcToBuildTime();
      renderRtcStatus();
      renderOledStatusDashboard();
    } else if (retry) {
      resetRtcStatus();
      showRtcStatus();
      renderOledStatusDashboard();
    }
  } else if (currentScreen == Screen::LoraDiag) {
    bool retry = keys.enter;

    for (char key : keys.word) {
      if (key == 'r' || key == 'R') {
        retry = true;
      }
    }

    if (retry) {
      stopLoraDiagnostics();
      resetLoraDiagnostics();
      showLoraDiag();
    }
  } else if (currentScreen == Screen::GnssDashboard) {
    bool reset = keys.enter;

    for (char key : keys.word) {
      if (key == 'r' || key == 'R') {
        reset = true;
      }
    }

    if (reset) {
      stopGnssDashboard();
      showGnssDashboard();
    }
  } else if (currentScreen == Screen::GnssSkyView) {
    bool reset = keys.enter;
    bool selectionChanged = false;

    for (char key : keys.word) {
      if (key == ';' || key == ',') {
        moveGnssSkySelection(-1);
        selectionChanged = true;
      } else if (key == '.' || key == '/') {
        moveGnssSkySelection(1);
        selectionChanged = true;
      } else if (key == 'r' || key == 'R') {
        reset = true;
      }
    }

    if (reset) {
      stopGnssSkyView();
      showGnssSkyView();
    } else if (selectionChanged) {
      renderGnssSkyView();
      renderOledStatusDashboard();
    }
  } else if (currentScreen == Screen::ReturnHome) {
    bool reset = keys.enter;
    bool changed = false;

    for (char key : keys.word) {
      if (key == 's' || key == 'S') {
        saveReturnHomeWaypoint();
        changed = true;
      } else if (key == 'd' || key == 'D') {
        clearReturnHomeWaypoint();
        changed = true;
      } else if (key == 'r' || key == 'R') {
        reset = true;
      }
    }

    if (reset) {
      stopReturnHome();
      showReturnHome();
    } else if (changed) {
      renderReturnHome();
      renderOledStatusDashboard();
    }
  } else if (currentScreen == Screen::BreadcrumbLogger) {
    bool reset = keys.enter;
    bool changed = false;

    for (char key : keys.word) {
      if (key == 's' || key == 'S') {
        toggleBreadcrumbLogging();
        changed = true;
      } else if (key == 'r' || key == 'R') {
        reset = true;
      }
    }

    if (reset) {
      stopBreadcrumbLogger();
      showBreadcrumbLogger();
    } else if (changed) {
      renderBreadcrumbLogger();
      renderOledStatusDashboard();
    }
  } else if (currentScreen == Screen::LoraPacketMonitor) {
    bool reset = keys.enter;
    bool changed = false;

    for (char key : keys.word) {
      if (key == 'r' || key == 'R') {
        reset = true;
      } else if (key == 'c' || key == 'C') {
        clearLoraPacketMonitor();
        changed = true;
      }
    }

    if (reset) {
      stopLoraPacketMonitor();
      showLoraPacketMonitor();
    } else if (changed) {
      renderLoraPacketMonitor();
      renderOledStatusDashboard();
    }
  } else if (currentScreen == Screen::LoraRangeTest) {
    bool reset = keys.enter;
    bool changed = false;

    for (char key : keys.word) {
      if (key == 'a' || key == 'A') {
        toggleLoraRangeArm();
        changed = true;
      } else if (key == 'p' || key == 'P') {
        sendLoraRangePing();
        changed = true;
      } else if (key == 'c' || key == 'C') {
        clearLoraRangeTest();
        changed = true;
      } else if (key == 'r' || key == 'R') {
        reset = true;
      }
    }

    if (reset) {
      stopLoraRangeTest();
      showLoraRangeTest();
    } else if (changed) {
      renderLoraRangeTest();
      renderOledStatusDashboard();
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
