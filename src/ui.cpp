#include "app.h"

void setScreen(Screen screen) {
  if (currentScreen == Screen::PiMonitor && screen != Screen::PiMonitor) {
    stopPiMonitor();
  }
  if (currentScreen == Screen::LoraDiag && screen != Screen::LoraDiag) {
    stopLoraDiagnostics();
  }
  if (currentScreen == Screen::GnssDashboard &&
      screen != Screen::GnssDashboard) {
    stopGnssDashboard();
  }
  if (currentScreen == Screen::GnssSkyView && screen != Screen::GnssSkyView) {
    stopGnssSkyView();
  }

  currentScreen = screen;
  clearOledStatusLine();
  lastSystemRefreshMs = 0;
  lastLevelRefreshMs = 0;
  lastOledRefreshMs = 0;
  lastEnvironmentRefreshMs = 0;
  lastLoraDiagRenderMs = 0;
  lastGnssDashboardRenderMs = 0;
  lastGnssSkyViewRenderMs = 0;

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
    case Screen::WifiConnect:
      drawScreenFrame("WiFi Connect");
      showWifiConnect();
      break;
    case Screen::PiMonitor:
      drawScreenFrame("Pi Monitor (T tgt C/I/S)");
      showPiMonitor();
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
    case Screen::OledTest:
      drawScreenFrame("OLED Test (PaHub ch1)");
      showOledTest();
      break;
    case Screen::GnssDashboard:
      drawScreenFrame("GNSS Dashboard");
      showGnssDashboard();
      break;
    case Screen::GnssSkyView:
      drawScreenFrame("GNSS Sky View");
      showGnssSkyView();
      break;
    case Screen::LoraDiag:
      drawScreenFrame("LoRa Diag (RX only)");
      showLoraDiag();
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

bool initContentCanvas() {
  const int width = M5Cardputer.Display.width();
  const int height = M5Cardputer.Display.height() - CONTENT_TOP;

  contentCanvas.deleteSprite();
  contentCanvas.setPsram(false);
  contentCanvas.setColorDepth(CONTENT_CANVAS_COLOR_DEPTH);
  contentCanvasReady = contentCanvas.createSprite(width, height) != nullptr;

  if (!contentCanvasReady) {
    contentCanvas.deleteSprite();
    contentCanvas.setColorDepth(CONTENT_CANVAS_FALLBACK_COLOR_DEPTH);
    contentCanvasReady = contentCanvas.createSprite(width, height) != nullptr;
  }

  if (contentCanvasReady) {
    contentCanvas.setFont(&fonts::Font2);
    contentCanvas.setTextSize(1);
    Serial.printf("Display: content canvas ready %dx%d depth=%u heap=%u\n",
                  width, height, contentCanvas.getColorDepth(), ESP.getFreeHeap());
  } else {
    Serial.printf("Display: content canvas allocation failed %dx%d heap=%u\n",
                  width, height, ESP.getFreeHeap());
  }

  return contentCanvasReady;
}

void beginContentDraw() {
  if (!contentCanvasReady || contentCanvas.getBuffer() == nullptr) {
    initContentCanvas();
  }

  if (!contentCanvasReady) {
    M5Cardputer.Display.fillRect(0, CONTENT_TOP, M5Cardputer.Display.width(),
                                 M5Cardputer.Display.height() - CONTENT_TOP, BLACK);
    M5Cardputer.Display.setTextColor(WHITE, BLACK);
    M5Cardputer.Display.setCursor(8, CONTENT_TOP + 4);
    M5Cardputer.Display.println("Display buffer error");
    M5Cardputer.Display.println("Try USB power/reboot.");
    return;
  }

  contentCanvas.fillScreen(BLACK);
  contentCanvas.setTextColor(WHITE, BLACK);
  contentCanvas.setCursor(8, 4);
}

void commitContentDraw() {
  if (!contentCanvasReady || contentCanvas.getBuffer() == nullptr) {
    return;
  }

  contentCanvas.pushSprite(&M5Cardputer.Display, 0, CONTENT_TOP);
}

void showMainMenu() {
  drawHeader("Scoober (Use arrows, OK to select)");

  if (selectedMenuIndex < menuScrollOffset) {
    menuScrollOffset = selectedMenuIndex;
  } else if (selectedMenuIndex >= menuScrollOffset + MAIN_MENU_VISIBLE_ROWS) {
    menuScrollOffset = selectedMenuIndex - MAIN_MENU_VISIBLE_ROWS + 1;
  }

  if (menuScrollOffset < 0) {
    menuScrollOffset = 0;
  }
  if (menuScrollOffset > MENU_ITEM_COUNT - MAIN_MENU_VISIBLE_ROWS) {
    menuScrollOffset = max(0, MENU_ITEM_COUNT - MAIN_MENU_VISIBLE_ROWS);
  }

  const int visibleCount = min(MAIN_MENU_VISIBLE_ROWS, MENU_ITEM_COUNT);
  for (int visibleIndex = 0; visibleIndex < visibleCount; ++visibleIndex) {
    const int menuIndex = menuScrollOffset + visibleIndex;
    const int rowY = 28 + (visibleIndex * 12);
    const bool selected = menuIndex == selectedMenuIndex;

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

    M5Cardputer.Display.print(MENU_ITEMS[menuIndex].label);
  }

  M5Cardputer.Display.setTextColor(DARKGREY, BLACK);
  M5Cardputer.Display.setCursor(182, 124);
  M5Cardputer.Display.printf("%d-%d/%d", menuScrollOffset + 1,
                             menuScrollOffset + visibleCount, MENU_ITEM_COUNT);
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
