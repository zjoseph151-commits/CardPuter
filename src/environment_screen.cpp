#include "app.h"

void showEnvironment() {
  lastEnvironmentRefreshMs = millis();
  const bool hasFreshReading = readEnvironmentSensor();
  if (hasFreshReading) {
    appendEnvironmentLogSample();
  }

  beginContentDraw();
  if (envLogging) {
    contentCanvas.printf("Log:%s #%lu\n", envLogFileName.c_str(), envLogSampleCount);
    contentCanvas.println("L stop log");
  } else {
    contentCanvas.println("L name log");
    if (envLogStatus.length() > 0) {
      contentCanvas.println(envLogStatus.substring(0, 28));
    }
  }

  if (!envSensorInitialized) {
    contentCanvas.println("ENV III not found");
    contentCanvas.println("Plug into Grove port.");
    contentCanvas.printf("SDA:G%d SCL:G%d\n", ENV_I2C_SDA_PIN, ENV_I2C_SCL_PIN);
    contentCanvas.println("Retrying...");
    commitContentDraw();
    return;
  }

  contentCanvas.printf("Status: %s\n", envStatus.c_str());

  if (envHasTempHumidity) {
    const float tempF = (envTemperatureC * 9.0f / 5.0f) + 32.0f;
    contentCanvas.printf("Temp: %.1f C / %.1f F\n", envTemperatureC, tempF);
    contentCanvas.printf("Humidity: %.1f %%\n", envHumidityPercent);
  } else {
    contentCanvas.println("Temp: waiting");
    contentCanvas.println("Humidity: waiting");
  }

  if (envHasPressure) {
    contentCanvas.printf("Pressure: %.1f hPa\n", envPressureHpa);
    contentCanvas.printf("Altitude: %.1f m\n", envAltitudeM);
  } else {
    contentCanvas.println("Pressure: waiting");
  }

  if (!hasFreshReading) {
    contentCanvas.println("Waiting for update...");
  }

  commitContentDraw();
}

void renderEnvironmentLogName() {
  beginContentDraw();
  contentCanvas.println("Name this log:");
  contentCanvas.println();

  if (envLogNameInput.length() > 0) {
    contentCanvas.printf("> %s\n", envLogNameInput.substring(0, 22).c_str());
  } else {
    contentCanvas.println("> (blank = envNNN)");
  }

  contentCanvas.println();
  contentCanvas.println("OK start");
  contentCanvas.println("Back del/cancel");
  contentCanvas.printf("%d/%d chars\n", envLogNameInput.length(),
                       ENV_LOG_NAME_MAX_LENGTH);
  commitContentDraw();
}

bool initEnvironmentSensor() {
  lastEnvironmentRetryMs = millis();
  Wire.begin(ENV_I2C_SDA_PIN, ENV_I2C_SCL_PIN, ENV_I2C_FREQUENCY);

  envSht30Ready =
      envSht30.begin(&Wire, SHT3X_I2C_ADDR, ENV_I2C_SDA_PIN, ENV_I2C_SCL_PIN,
                     ENV_I2C_FREQUENCY);
  envQmp6988Ready =
      envQmp6988.begin(&Wire, QMP6988_SLAVE_ADDRESS_L, ENV_I2C_SDA_PIN,
                       ENV_I2C_SCL_PIN, ENV_I2C_FREQUENCY);
  envSensorInitialized = envSht30Ready || envQmp6988Ready;

  if (!envSensorInitialized) {
    envHasTempHumidity = false;
    envHasPressure = false;
    envStatus = "ENV III not found";
    Serial.println("Environment: ENV III not found on Grove I2C.");
    return false;
  }

  if (envSht30Ready && envQmp6988Ready) {
    envStatus = "Connected";
  } else if (envSht30Ready) {
    envStatus = "SHT30 only";
  } else {
    envStatus = "QMP6988 only";
  }

  Serial.printf("Environment: %s (SHT30=%s QMP6988=%s)\n", envStatus.c_str(),
                envSht30Ready ? "ok" : "missing",
                envQmp6988Ready ? "ok" : "missing");
  return true;
}

bool readEnvironmentSensor() {
  if (!envSensorInitialized) {
    if (millis() - lastEnvironmentRetryMs > ENV_RETRY_INTERVAL_MS) {
      initEnvironmentSensor();
    }
    return false;
  }

  bool updated = false;

  if (envSht30Ready && envSht30.update()) {
    envTemperatureC = envSht30.cTemp;
    envHumidityPercent = envSht30.humidity;
    envHasTempHumidity = true;
    updated = true;
  }

  if (envQmp6988Ready && envQmp6988.update()) {
    envPressureHpa = envQmp6988.pressure * 0.01f;
    envAltitudeM = envQmp6988.altitude;
    envHasPressure = true;
    updated = true;
  }

  if (updated) {
    envStatus = "Connected";
    Serial.printf("Environment: %.1f C %.1f %% %.1f hPa\n", envTemperatureC,
                  envHumidityPercent, envPressureHpa);
  } else if (millis() - lastEnvironmentRetryMs > ENV_RETRY_INTERVAL_MS) {
    initEnvironmentSensor();
  }

  return updated;
}

bool initEnvironmentLogSd() {
  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

  if (!SD.begin(SD_SPI_CS_PIN, SPI, SD_SPI_FREQUENCY)) {
    envLogStatus = "SD init failed.";
    Serial.println("Environment log: SD init failed.");
    return false;
  }

  if (SD.cardType() == CARD_NONE) {
    envLogStatus = "No SD card.";
    Serial.println("Environment log: no SD card.");
    return false;
  }

  if (!SD.exists(ENV_LOG_DIR) && !SD.mkdir(ENV_LOG_DIR)) {
    envLogStatus = "Cannot make /env.";
    Serial.println("Environment log: could not create /env.");
    return false;
  }

  return true;
}

String sanitizeEnvironmentLogName(const String& requestedName) {
  String cleanName;

  for (int i = 0; i < requestedName.length() &&
                  cleanName.length() < ENV_LOG_NAME_MAX_LENGTH;
       ++i) {
    const char c = requestedName.charAt(i);

    if (isAlphaNumeric(c) || c == '_' || c == '-') {
      cleanName += c;
    } else if (c == ' ' && cleanName.length() > 0 &&
               cleanName.charAt(cleanName.length() - 1) != '_') {
      cleanName += '_';
    }
  }

  cleanName.trim();
  while (cleanName.endsWith("_")) {
    cleanName.remove(cleanName.length() - 1);
  }

  if (cleanName.length() == 0) {
    cleanName = "env";
  }

  return cleanName;
}

bool findNextEnvironmentLogPath(const String& requestedName, String& path, String& name) {
  if (!initEnvironmentLogSd()) {
    return false;
  }

  const String cleanName = sanitizeEnvironmentLogName(requestedName);

  for (int i = 1; i <= 999; ++i) {
    char filename[32];
    snprintf(filename, sizeof(filename), "%s%03d.csv", cleanName.c_str(), i);
    name = filename;
    path = String(ENV_LOG_DIR) + "/" + name;

    if (!SD.exists(path.c_str())) {
      return true;
    }
  }

  envLogStatus = "Log list full.";
  return false;
}

bool startEnvironmentLogging() {
  return startEnvironmentLogging("");
}

bool startEnvironmentLogging(const String& requestedName) {
  if (envLogging) {
    return true;
  }

  if (!findNextEnvironmentLogPath(requestedName, envLogFilePath, envLogFileName)) {
    return false;
  }

  envLogFile = SD.open(envLogFilePath.c_str(), FILE_WRITE);
  if (!envLogFile) {
    envLogStatus = "Log open failed.";
    Serial.printf("Environment log: open failed for %s\n", envLogFilePath.c_str());
    return false;
  }

  envLogFile.println(ENV_LOG_HEADER);
  envLogFile.flush();
  envLogSampleCount = 0;
  envLogging = true;
  envLogStatus = "Logging.";
  Serial.printf("Environment log started: %s\n", envLogFilePath.c_str());
  return true;
}

void stopEnvironmentLogging(const char* message) {
  if (!envLogging) {
    return;
  }

  envLogging = false;
  if (envLogFile) {
    envLogFile.flush();
    envLogFile.close();
  }

  envLogStatus = message;
  Serial.printf("Environment log stopped: %s (%lu samples)\n",
                envLogFilePath.c_str(), envLogSampleCount);
}

void appendEnvironmentLogSample() {
  if (!envLogging || !envLogFile) {
    return;
  }

  const float tempF = (envTemperatureC * 9.0f / 5.0f) + 32.0f;

  envLogFile.print(millis() / 1000UL);
  envLogFile.print(',');
  if (envHasTempHumidity) {
    envLogFile.print(envTemperatureC, 2);
  }
  envLogFile.print(',');
  if (envHasTempHumidity) {
    envLogFile.print(tempF, 2);
  }
  envLogFile.print(',');
  if (envHasTempHumidity) {
    envLogFile.print(envHumidityPercent, 2);
  }
  envLogFile.print(',');
  if (envHasPressure) {
    envLogFile.print(envPressureHpa, 2);
  }
  envLogFile.print(',');
  if (envHasPressure && isfinite(envAltitudeM)) {
    envLogFile.print(envAltitudeM, 2);
  }
  envLogFile.println();
  envLogFile.flush();

  envLogSampleCount++;
}
