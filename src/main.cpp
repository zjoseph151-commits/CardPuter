#include "app.h"

void setup() {
  Serial.begin(115200);
  delay(200);

  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setFont(&fonts::Font2);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.setTextColor(WHITE, BLACK);
  M5Cardputer.Display.fillScreen(BLACK);
  contentCanvas.setColorDepth(16);
  contentCanvas.createSprite(M5Cardputer.Display.width(),
                             M5Cardputer.Display.height() - CONTENT_TOP);
  contentCanvas.setFont(&fonts::Font2);
  contentCanvas.setTextSize(1);

  Serial.println();
  Serial.println(FIRMWARE_NAME);
  Serial.println(FIRMWARE_VERSION);
  loadSavedWifiNames();
  sampleBatteryStatus();

  drawHeader("M5 Cardputer Lab");
  M5Cardputer.Display.setCursor(8, 38);
  M5Cardputer.Display.println("Scoober");
  M5Cardputer.Display.println(FIRMWARE_VERSION);
  delay(1200);

  setScreen(Screen::MainMenu);
}

void loop() {
  M5Cardputer.update();
  handleKeyboard();

  const unsigned long now = millis();

  if (now - lastBatterySampleMs > 1000) {
    sampleBatteryStatus();
  }

  if ((currentScreen == Screen::BatteryInfo || currentScreen == Screen::SystemInfo) &&
      now - lastSystemRefreshMs > 1000) {
    if (currentScreen == Screen::BatteryInfo) {
      showBatteryInfo();
    } else {
      showSystemInfo();
    }
  }

  if (currentScreen == Screen::Environment &&
      now - lastEnvironmentRefreshMs > ENV_REFRESH_INTERVAL_MS) {
    showEnvironment();
  }

  if (currentScreen == Screen::LevelTool && now - lastLevelRefreshMs > 80) {
    showLevelTool();
  }

  if (currentScreen == Screen::VoiceMemos && voiceMemoRecording) {
    serviceVoiceMemoRecording();
  }

  delay(10);
}
