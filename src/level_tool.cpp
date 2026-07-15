#include "app.h"

void showLevelTool() {
  lastLevelRefreshMs = millis();

  beginContentDraw();

  M5.Imu.update();

  if (!M5.Imu.isEnabled()) {
    contentCanvas.println("IMU unavailable.");
    contentCanvas.println("Cardputer-Adv only.");
    commitContentDraw();
    return;
  }

  m5::imu_data_t imuData = M5.Imu.getImuData();
  const float rawLevelX = imuData.accel.x;
  const float rawLevelY = imuData.accel.y;

  if (!levelSmoothingInitialized) {
    smoothedLevelX = rawLevelX;
    smoothedLevelY = rawLevelY;
    levelSmoothingInitialized = true;
  } else {
    smoothedLevelX += (rawLevelX - smoothedLevelX) * LEVEL_SMOOTHING;
    smoothedLevelY += (rawLevelY - smoothedLevelY) * LEVEL_SMOOTHING;
  }

  const int centerX = contentCanvas.width() / 2;
  const int centerY = contentCanvas.height() / 2;
  const int dotX = constrain(centerX + static_cast<int>(smoothedLevelX * LEVEL_DOT_SCALE_PIXELS),
                             8, contentCanvas.width() - 8);
  const int dotY = constrain(centerY + static_cast<int>(smoothedLevelY * LEVEL_DOT_SCALE_PIXELS),
                             8, contentCanvas.height() - 8);
  const int dx = dotX - centerX;
  const int dy = dotY - centerY;
  const bool isLevel = (dx * dx + dy * dy) <=
                       (LEVEL_TOLERANCE_PIXELS * LEVEL_TOLERANCE_PIXELS);

  const float pitchDeg =
      atan2f(-imuData.accel.x,
             sqrtf((imuData.accel.y * imuData.accel.y) +
                   (imuData.accel.z * imuData.accel.z))) *
      LEVEL_RAD_TO_DEG;
  const float rollDeg = atan2f(imuData.accel.y, imuData.accel.z) * LEVEL_RAD_TO_DEG;

  drawLevelCrosshair(centerX, centerY);
  drawLevelDot(dotX, dotY, isLevel);

  contentCanvas.setTextColor(isLevel ? GREEN : YELLOW, BLACK);
  contentCanvas.setCursor(8, 4);
  contentCanvas.print(isLevel ? "LEVEL" : "TILT");
  contentCanvas.setTextColor(WHITE, BLACK);
  contentCanvas.setCursor(8, contentCanvas.height() - 16);
  contentCanvas.printf("P:%+.1f R:%+.1f", pitchDeg, rollDeg);
  commitContentDraw();
}

void resetLevelSmoothing() {
  smoothedLevelX = 0.0f;
  smoothedLevelY = 0.0f;
  levelSmoothingInitialized = false;
}

void drawLevelCrosshair(int centerX, int centerY) {
  contentCanvas.drawCircle(centerX, centerY, 42, DARKGREY);
  contentCanvas.drawCircle(centerX, centerY, LEVEL_TOLERANCE_PIXELS, GREEN);
  contentCanvas.drawLine(centerX - 52, centerY, centerX + 52, centerY, DARKGREY);
  contentCanvas.drawLine(centerX, centerY - 42, centerX, centerY + 42, DARKGREY);
  contentCanvas.drawLine(centerX - 6, centerY, centerX + 6, centerY, GREEN);
  contentCanvas.drawLine(centerX, centerY - 6, centerX, centerY + 6, GREEN);
}

void drawLevelDot(int dotX, int dotY, bool isLevel) {
  const uint16_t color = isLevel ? GREEN : ORANGE;
  contentCanvas.fillCircle(dotX, dotY, 5, color);
  contentCanvas.drawCircle(dotX, dotY, 6, WHITE);
}
