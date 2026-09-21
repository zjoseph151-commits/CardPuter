#include "app.h"

#include <RadioLib.h>
#include "utility/PI4IOE5V6408_Class.hpp"

namespace {

m5::PI4IOE5V6408_Class loraRangeIoExpander(LORA_IO_EXPANDER_ADDRESS, 400000,
                                            &m5::In_I2C);
SX1262 loraRangeRadio = new Module(LORA_NSS_PIN, LORA_IRQ_PIN, LORA_RST_PIN,
                                    LORA_BUSY_PIN);
volatile bool loraRangeDio1Pending = false;
bool loraRangeIoExpanderDetected = false;
uint32_t loraRangePendingSequence = 0;
unsigned long loraRangeTxStartedMs = 0;
unsigned long loraRangeAckWaitStartedMs = 0;
float loraRangeAttemptAckRssi = NAN;
float loraRangeAttemptAckSnr = NAN;
float loraRangeAttemptRemoteRssi = NAN;
float loraRangeAttemptRemoteSnr = NAN;

#if defined(ESP32)
void IRAM_ATTR setLoraRangeDio1Flag() {
#else
void setLoraRangeDio1Flag() {
#endif
  loraRangeDio1Pending = true;
}

String clippedRangeText(const String& text, int maxChars) {
  return text.substring(0, maxChars);
}

String rangeMetricText(float value) {
  if (!isfinite(value)) {
    return "--";
  }

  char text[12];
  snprintf(text, sizeof(text), "%.1f", value);
  return text;
}

String loraRangeAckAgeText() {
  if (loraRangeLastAckMs == 0) {
    return "--";
  }

  return String((millis() - loraRangeLastAckMs) / 1000UL) + "s";
}

String rangeCsvField(const String& payload, uint8_t index) {
  int start = 0;
  uint8_t current = 0;
  while (start <= payload.length()) {
    const int end = payload.indexOf(',', start);
    const int fieldEnd = end >= 0 ? end : payload.length();
    if (current == index) {
      return payload.substring(start, fieldEnd);
    }
    if (end < 0) {
      break;
    }
    start = end + 1;
    ++current;
  }
  return "";
}

String rangeUtcCsvText() {
  char text[12];
  if (loraGnssTimeValid) {
    snprintf(text, sizeof(text), "%02u:%02u:%02u", loraGnssHour,
             loraGnssMinute, loraGnssSecond);
    return text;
  }
  if (rtcTimeValid) {
    snprintf(text, sizeof(text), "%02u:%02u:%02u", rtcHour, rtcMinute,
             rtcSecond);
    return text;
  }
  return "";
}

String rangeDateCsvText() {
  char text[12];
  if (loraGnssDateValid) {
    snprintf(text, sizeof(text), "%04u-%02u-%02u", loraGnssYear,
             loraGnssMonth, loraGnssDay);
    return text;
  }
  if (rtcTimeValid && rtcYear > 0) {
    snprintf(text, sizeof(text), "%04u-%02u-%02u", rtcYear, rtcMonth,
             rtcDay);
    return text;
  }
  return "";
}

const char* rangeTimeSource() {
  if (loraGnssTimeValid && loraGnssDateValid) {
    return "gnss";
  }
  if (rtcTimeValid) {
    return "rtc";
  }
  return "uptime";
}

void prepareSharedExtSpiForRange() {
  if (sharedSpiOwner == SHARED_SPI_OWNER_LORA) {
    deselectSharedSpiDevices();
    return;
  }

  deselectSharedSpiDevices();
  SPI.end();
  delay(2);
  deselectSharedSpiDevices();
  SPI.begin(LORA_SPI_SCK_PIN, LORA_SPI_MISO_PIN, LORA_SPI_MOSI_PIN,
            LORA_NSS_PIN);
  deselectSharedSpiDevices();
  sharedSpiOwner = SHARED_SPI_OWNER_LORA;
}

bool initLoraRangeSd() {
  prepareSharedSpiForSd();

  if (!SD.begin(SD_SPI_CS_PIN, SPI, SD_SPI_FREQUENCY)) {
    sharedSpiOwner = SHARED_SPI_OWNER_NONE;
    loraRangeLogStatus = "SD init failed.";
    return false;
  }

  if (SD.cardType() == CARD_NONE) {
    loraRangeLogStatus = "No SD card.";
    return false;
  }

  if (!SD.exists(LORA_RANGE_LOG_DIR) && !SD.mkdir(LORA_RANGE_LOG_DIR)) {
    loraRangeLogStatus = "Cannot make /tracks.";
    return false;
  }

  return true;
}

bool findNextLoraRangeLogPath(String& path, String& name) {
  for (int i = 1; i <= 999; ++i) {
    char filename[28];
    snprintf(filename, sizeof(filename), "lora-range%03d.csv", i);
    name = filename;
    path = String(LORA_RANGE_LOG_DIR) + "/" + name;
    if (!SD.exists(path.c_str())) {
      return true;
    }
  }

  loraRangeLogStatus = "Range log list full.";
  return false;
}

void restoreLoraRangeSpi() {
  if (!loraRangeRadioReady) {
    return;
  }
  prepareSharedExtSpiForRange();
}

bool startLoraRangeReceive(const String& readyStatus) {
  if (!loraRangeRadioReady) {
    return false;
  }

  loraRangeDio1Pending = false;
  loraRangeRadioState = loraRangeRadio.startReceive();
  loraRangeListening = loraRangeRadioState == RADIOLIB_ERR_NONE;
  if (loraRangeListening) {
    loraRangeStatus = readyStatus;
    return true;
  }

  loraRangeStatus = String("Listen failed ") + loraRangeRadioState;
  Serial.printf("LoRa Range: startReceive failed code=%d.\n",
                loraRangeRadioState);
  return false;
}

bool createLoraRangeLog() {
  if (loraRangeLogFileName.length() > 0) {
    return true;
  }

  if (loraRangeRadioReady) {
    loraRangeRadio.standby();
    loraRangeListening = false;
  }

  if (!initLoraRangeSd()) {
    restoreLoraRangeSpi();
    startLoraRangeReceive("Listening; SD unavailable");
    return false;
  }

  String path;
  String name;
  if (!findNextLoraRangeLogPath(path, name)) {
    restoreLoraRangeSpi();
    startLoraRangeReceive("Listening; log unavailable");
    return false;
  }

  File file = SD.open(path.c_str(), FILE_WRITE);
  if (!file) {
    loraRangeLogStatus = "Range log open failed.";
    restoreLoraRangeSpi();
    startLoraRangeReceive("Listening; log unavailable");
    return false;
  }

  file.println(LORA_RANGE_LOG_HEADER);
  file.close();
  loraRangeLogFileName = name;
  loraRangeLogStatus = "CSV ready.";
  Serial.printf("LoRa Range: created %s.\n", path.c_str());

  restoreLoraRangeSpi();
  startLoraRangeReceive("Listening; ready to arm");
  return true;
}

bool appendLoraRangeLogRow(const char* result) {
  if (loraRangeLogFileName.length() == 0) {
    loraRangeLogStatus = "No CSV log.";
    return false;
  }

  if (loraRangeRadioReady) {
    loraRangeRadio.standby();
    loraRangeListening = false;
  }

  if (!initLoraRangeSd()) {
    restoreLoraRangeSpi();
    return false;
  }

  const String path = String(LORA_RANGE_LOG_DIR) + "/" + loraRangeLogFileName;
  File file = SD.open(path.c_str(), FILE_WRITE);
  if (!file) {
    loraRangeLogStatus = "Range log write failed.";
    restoreLoraRangeSpi();
    return false;
  }

  file.print(millis() / 1000UL);
  file.print(',');
  file.print(rangeUtcCsvText());
  file.print(',');
  file.print(rangeDateCsvText());
  file.print(',');
  file.print(rangeTimeSource());
  file.print(',');
  file.print(loraRangePendingSequence);
  file.print(',');
  file.print(result);
  file.print(',');
  file.print(loraRangeRadioState);
  file.print(',');
  file.print(isfinite(loraRangeAttemptAckRssi)
                 ? String(loraRangeAttemptAckRssi, 1)
                 : String(""));
  file.print(',');
  file.print(isfinite(loraRangeAttemptAckSnr)
                 ? String(loraRangeAttemptAckSnr, 1)
                 : String(""));
  file.print(',');
  file.print(isfinite(loraRangeAttemptRemoteRssi)
                 ? String(loraRangeAttemptRemoteRssi, 1)
                 : String(""));
  file.print(',');
  file.print(isfinite(loraRangeAttemptRemoteSnr)
                 ? String(loraRangeAttemptRemoteSnr, 1)
                 : String(""));
  file.print(',');
  file.println(lastBatteryLevel);
  file.close();

  ++loraRangeLogRowCount;
  loraRangeLogStatus = String("Logged ") + result;
  restoreLoraRangeSpi();
  return true;
}

void finishLoraRangeTransmit() {
  loraRangeDio1Pending = false;
  loraRangeTransmitting = false;
  loraRangeRadioState = loraRangeRadio.finishTransmit();

  if (loraRangeRadioState != RADIOLIB_ERR_NONE) {
    ++loraRangeTxFailCount;
    loraRangeStatus = String("TX finish failed ") + loraRangeRadioState;
    appendLoraRangeLogRow("tx_finish_error");
    startLoraRangeReceive(loraRangeStatus);
    return;
  }

  ++loraRangeTxCount;
  loraRangeSequence = loraRangePendingSequence;
  loraRangeAwaitingAck = true;
  loraRangeAckWaitStartedMs = millis();
  const String status = String("Ping ") + loraRangePendingSequence +
                        " sent; wait ACK";
  startLoraRangeReceive(status);
  Serial.printf("LoRa Range: ping #%lu sent; awaiting ACK.\n",
                static_cast<unsigned long>(loraRangePendingSequence));
}

void handleLoraRangeReceive() {
  loraRangeDio1Pending = false;

  String packet;
  loraRangeRadioState = loraRangeRadio.readData(packet);
  if (loraRangeRadioState == RADIOLIB_ERR_NONE) {
    loraRangeAttemptAckRssi = loraRangeRadio.getRSSI();
    loraRangeAttemptAckSnr = loraRangeRadio.getSNR();

    const String ackSequence = rangeCsvField(packet, 4);
    if (packet.startsWith("SCBR,ACK,1,") && loraRangeAwaitingAck &&
        ackSequence == String(loraRangePendingSequence)) {
      loraRangeAwaitingAck = false;
      ++loraRangeAckCount;
      loraRangeLastAckMs = millis();
      loraRangeLastAckRssi = loraRangeAttemptAckRssi;
      loraRangeLastAckSnr = loraRangeAttemptAckSnr;
      loraRangeAttemptRemoteRssi = rangeCsvField(packet, 5).toFloat();
      loraRangeAttemptRemoteSnr = rangeCsvField(packet, 6).toFloat();
      loraRangeStatus = String("ACK seq ") + loraRangePendingSequence;
      appendLoraRangeLogRow("ack");
      startLoraRangeReceive(loraRangeStatus);
      Serial.printf("LoRa Range: ACK #%lu RSSI=%.1f SNR=%.1f.\n",
                    static_cast<unsigned long>(loraRangeAckCount),
                    loraRangeLastAckRssi, loraRangeLastAckSnr);
      return;
    }

    if (packet.startsWith("SCBR,ACK,1,")) {
      loraRangeStatus = "ACK sequence mismatch";
    } else {
      loraRangeStatus = "RX non-ACK packet";
    }
  } else if (loraRangeRadioState == RADIOLIB_ERR_CRC_MISMATCH) {
    loraRangeStatus = "ACK CRC mismatch";
  } else {
    loraRangeStatus = String("ACK RX error ") + loraRangeRadioState;
  }

  const String status = loraRangeAwaitingAck
                            ? String("Waiting ACK ") + loraRangePendingSequence
                            : loraRangeStatus;
  startLoraRangeReceive(status);
}

void checkLoraRangeTimeouts() {
  const unsigned long now = millis();
  if (loraRangeTransmitting &&
      now - loraRangeTxStartedMs >= LORA_RANGE_TX_TIMEOUT_MS) {
    loraRangeTransmitting = false;
    loraRangeRadioState = loraRangeRadio.finishTransmit();
    ++loraRangeTxFailCount;
    loraRangeStatus = "TX timeout";
    appendLoraRangeLogRow("tx_timeout");
    startLoraRangeReceive(loraRangeStatus);
    return;
  }

  if (loraRangeAwaitingAck &&
      now - loraRangeAckWaitStartedMs >= LORA_RANGE_ACK_WINDOW_MS) {
    loraRangeAwaitingAck = false;
    ++loraRangeAckTimeoutCount;
    loraRangeStatus = String("ACK timeout seq ") + loraRangePendingSequence;
    appendLoraRangeLogRow("ack_timeout");
    startLoraRangeReceive(loraRangeStatus);
    Serial.printf("LoRa Range: ACK timeout for seq %lu.\n",
                  static_cast<unsigned long>(loraRangePendingSequence));
  }
}

}  // namespace

void showLoraRangeTest() {
  if (!loraRangeInitialized) {
    initLoraRangeTest();
  }

  serviceLoraRangeTest();
  renderLoraRangeTest();
}

void renderLoraRangeTest() {
  lastLoraRangeRenderMs = millis();
  beginContentDraw();

  contentCanvas.printf("Arm:%s TX:%lu ACK:%lu\n",
                       loraRangeArmed ? "ON" : "OFF",
                       static_cast<unsigned long>(loraRangeTxCount),
                       static_cast<unsigned long>(loraRangeAckCount));
  contentCanvas.printf("Fail:%lu TO:%lu Seq:%lu\n",
                       static_cast<unsigned long>(loraRangeTxFailCount),
                       static_cast<unsigned long>(loraRangeAckTimeoutCount),
                       static_cast<unsigned long>(loraRangeSequence));
  contentCanvas.printf("Radio:%s\n",
                       clippedRangeText(loraRangeStatus, 24).c_str());
  contentCanvas.printf("AckR:%s S:%s Age:%s\n",
                       rangeMetricText(loraRangeLastAckRssi).c_str(),
                       rangeMetricText(loraRangeLastAckSnr).c_str(),
                       loraRangeAckAgeText().c_str());
  contentCanvas.printf("915.0 BW125 SF12 2dBm\n");
  contentCanvas.printf("Log:%s #%lu\n",
                       clippedRangeText(loraRangeLogFileName.length() > 0
                                            ? loraRangeLogFileName
                                            : String("none"),
                                        16)
                           .c_str(),
                       static_cast<unsigned long>(loraRangeLogRowCount));
  contentCanvas.println(clippedRangeText(loraRangeLogStatus, 24));
  contentCanvas.println("A arm  P ping  C clear");
  contentCanvas.println("OK/R restart  Back sleep");

  commitContentDraw();
}

bool initLoraRangeTest() {
  resetLoraRangeTest();
  loraRangeInitialized = true;
  loraRangeStatus = "Starting RX; antenna required";

  Serial.println("LoRa Range: starting manual ping/range test.");
  prepareSharedExtSpiForRange();
  startLoraGnssSerial();

  const bool internalI2cReady =
      m5::In_I2C.isEnabled() || m5::In_I2C.begin();
  if (!internalI2cReady) {
    loraRangeStatus = "I2C failed";
    return false;
  }

  if (!loraRangeIoExpander.begin()) {
    loraRangeStatus = "LoRa I/O not found";
    return false;
  }

  loraRangeIoExpanderDetected = true;
  loraRangeIoExpander.setDirection(LORA_RF_SWITCH_PIN, true);
  loraRangeIoExpander.setHighImpedance(LORA_RF_SWITCH_PIN, false);
  loraRangeIoExpander.digitalWrite(LORA_RF_SWITCH_PIN, true);

  loraRangeRadioState =
      loraRangeRadio.begin(LORA_DIAG_RX_FREQUENCY_MHZ,
                           LORA_DIAG_BANDWIDTH_KHZ,
                           LORA_DIAG_SPREADING_FACTOR,
                           LORA_DIAG_CODING_RATE, LORA_DIAG_SYNC_WORD,
                           LORA_DIAG_UNUSED_TX_POWER_DBM,
                           LORA_DIAG_PREAMBLE_LEN, 3.0, true);
  if (loraRangeRadioState != RADIOLIB_ERR_NONE) {
    loraRangeStatus = String("Init failed ") + loraRangeRadioState;
    Serial.printf("LoRa Range: SX1262 init failed code=%d.\n",
                  loraRangeRadioState);
    return false;
  }

  loraRangeRadioReady = true;
  loraRangeRadio.setCurrentLimit(140);
  loraRangeRadio.setDio1Action(setLoraRangeDio1Flag);
  startLoraRangeReceive("Listening; A arms + opens CSV");
  return loraRangeListening;
}

void serviceLoraRangeTest() {
  if (!loraRangeInitialized) {
    return;
  }

  const unsigned long now = millis();
  if (now - lastLoraRangeServiceMs < LORA_RANGE_SERVICE_INTERVAL_MS) {
    return;
  }

  lastLoraRangeServiceMs = now;
  serviceLoraGnssSerial();

  if (loraRangeDio1Pending) {
    if (loraRangeTransmitting) {
      finishLoraRangeTransmit();
    } else {
      handleLoraRangeReceive();
    }
  }

  checkLoraRangeTimeouts();

  if (currentScreen == Screen::LoraRangeTest &&
      now - lastLoraRangeRenderMs >= LORA_RANGE_RENDER_INTERVAL_MS) {
    renderLoraRangeTest();
  }
}

void resetLoraRangeTest() {
  loraRangeInitialized = false;
  loraRangeRadioReady = false;
  loraRangeListening = false;
  loraRangeArmed = false;
  loraRangeTransmitting = false;
  loraRangeAwaitingAck = false;
  loraRangeDio1Pending = false;
  loraRangeIoExpanderDetected = false;
  loraRangePendingSequence = 0;
  loraRangeTxStartedMs = 0;
  loraRangeAckWaitStartedMs = 0;
  loraRangeLastTxMs = 0;
  loraRangeLastAckMs = 0;
  lastLoraRangeServiceMs = 0;
  lastLoraRangeRenderMs = 0;
  loraRangeStatus = "Not initialized.";
  loraRangeLogStatus = "A arm to create CSV.";
  loraRangeLogFileName = "";
  loraRangeRadioState = 0;
  loraRangeLastAckRssi = NAN;
  loraRangeLastAckSnr = NAN;
  loraRangeAttemptAckRssi = NAN;
  loraRangeAttemptAckSnr = NAN;
  loraRangeAttemptRemoteRssi = NAN;
  loraRangeAttemptRemoteSnr = NAN;
  loraRangeTxCount = 0;
  loraRangeTxFailCount = 0;
  loraRangeAckCount = 0;
  loraRangeAckTimeoutCount = 0;
  loraRangeLogRowCount = 0;
  loraRangeSequence = 0;
  resetLoraGnssParser();
}

void stopLoraRangeTest() {
  if (loraRangeRadioReady) {
    loraRangeRadio.clearDio1Action();
    loraRangeRadio.sleep();
  }

  deselectSharedSpiDevices();
  stopLoraGnssSerial();
  resetLoraRangeTest();
}

void toggleLoraRangeArm() {
  if (!loraRangeRadioReady) {
    loraRangeStatus = "Radio unavailable";
    return;
  }

  if (loraRangeArmed) {
    loraRangeArmed = false;
    loraRangeStatus = "Disarmed; no TX";
    return;
  }

  if (!createLoraRangeLog()) {
    loraRangeStatus = "Arm blocked: CSV required";
    return;
  }

  loraRangeArmed = true;
  loraRangeStatus = "ARMED; P sends one ping";
  Serial.println("LoRa Range: armed for manual pings.");
}

void sendLoraRangePing() {
  if (!loraRangeRadioReady) {
    loraRangeStatus = "Radio unavailable";
    return;
  }
  if (!loraRangeArmed) {
    loraRangeStatus = "Press A to arm first";
    return;
  }
  if (loraRangeTransmitting || loraRangeAwaitingAck) {
    loraRangeStatus = "Wait for current result";
    return;
  }

  const unsigned long now = millis();
  if (loraRangeLastTxMs > 0 &&
      now - loraRangeLastTxMs < LORA_RANGE_SEND_COOLDOWN_MS) {
    const unsigned long remaining =
        (LORA_RANGE_SEND_COOLDOWN_MS - (now - loraRangeLastTxMs) + 999) /
        1000;
    loraRangeStatus = String("Rate limit: wait ") + remaining + "s";
    return;
  }

  loraRangePendingSequence = loraRangeSequence + 1;
  loraRangeAttemptAckRssi = NAN;
  loraRangeAttemptAckSnr = NAN;
  loraRangeAttemptRemoteRssi = NAN;
  loraRangeAttemptRemoteSnr = NAN;
  String payload = String("SCBR,PING,1,") + LORA_RANGE_DEVICE_ID + "," +
                   loraRangePendingSequence + "," + String(now) + "," +
                   String(lastBatteryLevel);

  loraRangeDio1Pending = false;
  loraRangeListening = false;
  loraRangeRadioState = loraRangeRadio.startTransmit(payload);
  if (loraRangeRadioState != RADIOLIB_ERR_NONE) {
    ++loraRangeTxFailCount;
    loraRangeStatus = String("TX start failed ") + loraRangeRadioState;
    appendLoraRangeLogRow("tx_start_error");
    startLoraRangeReceive(loraRangeStatus);
    return;
  }

  loraRangeTransmitting = true;
  loraRangeTxStartedMs = now;
  loraRangeLastTxMs = now;
  loraRangeStatus = String("TX ping ") + loraRangePendingSequence;
  Serial.printf("LoRa Range: starting ping #%lu: %s\n",
                static_cast<unsigned long>(loraRangePendingSequence),
                payload.c_str());
}

void clearLoraRangeTest() {
  loraRangeTxCount = 0;
  loraRangeTxFailCount = 0;
  loraRangeAckCount = 0;
  loraRangeAckTimeoutCount = 0;
  loraRangeLogRowCount = 0;
  loraRangeLastAckMs = 0;
  loraRangeLastAckRssi = NAN;
  loraRangeLastAckSnr = NAN;
  loraRangeStatus = "Counters cleared";
}
