#include "app.h"

bool detectI2cHub() {
  i2cHubDetected = false;
  i2cHubActiveChannel = -1;

  Wire.beginTransmission(I2C_HUB_ADDRESS);
  const uint8_t result = Wire.endTransmission();

  if (result == 0) {
    i2cHubDetected = true;
    i2cHubStatus = "PaHub 0x70";
    Serial.println("I2C Hub: M5Stack PaHub detected at 0x70.");
    return true;
  }

  i2cHubStatus = "Direct Grove";
  Serial.println("I2C Hub: no PaHub detected; using direct Grove I2C.");
  return false;
}

bool selectI2cHubChannel(uint8_t channel) {
  if (!i2cHubDetected) {
    i2cHubStatus = "Direct Grove";
    return true;
  }

  if (channel >= I2C_HUB_CHANNEL_COUNT) {
    i2cHubActiveChannel = -1;
    i2cHubStatus = "Bad PaHub channel";
    Serial.printf("I2C Hub: invalid channel %u.\n", channel);
    return false;
  }

  Wire.beginTransmission(I2C_HUB_ADDRESS);
  Wire.write(1U << channel);
  const uint8_t result = Wire.endTransmission();

  if (result == 0) {
    i2cHubActiveChannel = channel;
    i2cHubStatus = String("PaHub ch") + String(channel);
    return true;
  }

  i2cHubActiveChannel = -1;
  i2cHubStatus = "PaHub select failed";
  Serial.printf("I2C Hub: could not select channel %u (err %u).\n", channel,
                result);
  return false;
}

bool selectEnvironmentI2cPath() {
  if (!i2cHubDetected) {
    i2cHubStatus = "Direct Grove";
    return true;
  }

  return selectI2cHubChannel(I2C_HUB_ENV_CHANNEL);
}

String environmentI2cPathLabel() {
  if (!i2cHubDetected) {
    return "Direct Grove";
  }

  if (i2cHubActiveChannel >= 0) {
    return String("PaHub ch") + String(i2cHubActiveChannel);
  }

  return "PaHub";
}
