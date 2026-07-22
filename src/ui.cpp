#include "app.h"

void setScreen(Screen screen) {
  if (currentScreen == Screen::RfScanner && screen != Screen::RfScanner) {
    powerDownNrf24Radio();
  }

  currentScreen = screen;
  lastSystemRefreshMs = 0;
  lastLevelRefreshMs = 0;
  lastEnvironmentRefreshMs = 0;

  switch (currentScreen) {
    case Screen::MainMenu:
      showMainMenu();
      break;
    case Screen::BatteryInfo:
      sampleBatteryStatus();
      drawScreenFrame("Battery");
      showBatteryInfo();
      break;
    case Screen::SystemInfo:
      drawScreenFrame("System");
      showSystemInfo();
      break;
    case Screen::WifiScan:
      drawScreenFrame("WiFi Scan");
      showWifiScan();
      break;
    case Screen::WifiSaveConfirm:
      wifiSaveMessage = "";
      drawScreenFrame("Save WiFi");
      renderWifiSaveConfirm();
      break;
    case Screen::WifiSaveResult:
      drawScreenFrame("Save WiFi");
      renderWifiSaveResult();
      break;
    case Screen::SavedWifi:
      drawScreenFrame("Saved WiFi");
      renderSavedWifiList();
      break;
    case Screen::SavedWifiDeleteConfirm:
      drawScreenFrame("Delete WiFi");
      renderSavedWifiDeleteConfirm();
      break;
    case Screen::SavedWifiDeleteResult:
      drawScreenFrame("Delete WiFi");
      renderSavedWifiDeleteResult();
      break;
    case Screen::VoiceMemos:
      drawScreenFrame("Voice Memos (R record OK play)");
      showVoiceMemos();
      break;
    case Screen::VoiceMemoDeleteConfirm:
      drawScreenFrame("Delete Memo");
      renderVoiceMemoDeleteConfirm();
      break;
    case Screen::VoiceMemoDeleteResult:
      drawScreenFrame("Delete Memo");
      renderVoiceMemoDeleteResult();
      break;
    case Screen::Environment:
      drawScreenFrame("Environment (ENV III Unit)");
      initEnvironmentSensor();
      showEnvironment();
      break;
    case Screen::EnvironmentLogName:
      envLogNameInput = "";
      drawScreenFrame("Log Name");
      renderEnvironmentLogName();
      break;
    case Screen::RfScanner:
      drawScreenFrame("RF Scan");
      initNrf24Radio();
      showRfScanner();
      break;
    case Screen::LevelTool:
      resetLevelSmoothing();
      drawScreenFrame("Level");
      showLevelTool();
      break;
  }
}

void drawHeader(const char* title) {
  M5Cardputer.Display.fillScreen(BLACK);
  M5Cardputer.Display.fillRect(0, 0, M5Cardputer.Display.width(), HEADER_HEIGHT, NAVY);
  M5Cardputer.Display.setTextColor(WHITE, NAVY);
  M5Cardputer.Display.setCursor(6, 4);
  M5Cardputer.Display.print(title);
  M5Cardputer.Display.setTextColor(WHITE, BLACK);
}

void drawScreenFrame(const char* title) {
  drawHeader(title);
}

void beginContentDraw() {
  contentCanvas.fillScreen(BLACK);
  contentCanvas.setTextColor(WHITE, BLACK);
  contentCanvas.setCursor(8, 4);
}

void commitContentDraw() {
  contentCanvas.pushSprite(0, CONTENT_TOP);
}

void showMainMenu() {
  drawHeader("Scoober (Use arrows, OK to select)");

  for (int i = 0; i < MENU_ITEM_COUNT; ++i) {
    const int rowY = 30 + (i * 13);
    const bool selected = i == selectedMenuIndex;

    if (selected) {
      M5Cardputer.Display.fillRect(6, rowY - 1, M5Cardputer.Display.width() - 12, 12,
                                   DARKGREEN);
      M5Cardputer.Display.setTextColor(WHITE, DARKGREEN);
      M5Cardputer.Display.setCursor(12, rowY);
      M5Cardputer.Display.print("> ");
    } else {
      M5Cardputer.Display.setTextColor(WHITE, BLACK);
      M5Cardputer.Display.setCursor(22, rowY);
    }

    M5Cardputer.Display.print(MENU_ITEMS[i].label);
  }

  M5Cardputer.Display.setTextColor(WHITE, BLACK);
}

void moveMenuSelection(int direction) {
  selectedMenuIndex += direction;

  if (selectedMenuIndex < 0) {
    selectedMenuIndex = MENU_ITEM_COUNT - 1;
  } else if (selectedMenuIndex >= MENU_ITEM_COUNT) {
    selectedMenuIndex = 0;
  }

  showMainMenu();
}

void activateSelectedMenuItem() {
  setScreen(MENU_ITEMS[selectedMenuIndex].screen);
}
