#include "app.h"

#include <RadioLib.h>
#include "utility/PI4IOE5V6408_Class.hpp"

namespace {

m5::PI4IOE5V6408_Class loraIoExpander(LORA_IO_EXPANDER_ADDRESS, 400000,
                                       &m5::In_I2C);
SX1262 loraRadio = new Module(LORA_NSS_PIN, LORA_IRQ_PIN, LORA_RST_PIN,
                              LORA_BUSY_PIN);
HardwareSerial loraGnssSerial(1);
volatile bool loraPacketReceived = false;
String loraGnssLineBuffer;

#if defined(ESP32)
void IRAM_ATTR setLoraPacketReceivedFlag() {
#else
void setLoraPacketReceivedFlag() {
#endif
  loraPacketReceived = true;
}

String clippedLoraText(const String& text, int maxChars) {
  return text.substring(0, maxChars);
}

String loraMetricText(float value, uint32_t count) {
  if (count == 0) {
    return "--";
  }

  char metric[12];
  snprintf(metric, sizeof(metric), "%.1f", value);
  return metric;
}

void prepareSharedExtSpiForLora() {
  pinMode(SD_SPI_CS_PIN, OUTPUT);
  digitalWrite(SD_SPI_CS_PIN, HIGH);
  pinMode(LORA_NSS_PIN, OUTPUT);
  digitalWrite(LORA_NSS_PIN, HIGH);
  SPI.begin(LORA_SPI_SCK_PIN, LORA_SPI_MISO_PIN, LORA_SPI_MOSI_PIN,
            LORA_NSS_PIN);
}

void startLoraGnssSerial() {
  if (loraGnssStarted) {
    return;
  }

  loraGnssSerial.begin(LORA_GNSS_BAUD, SERIAL_8N1, LORA_GNSS_RX_PIN,
                       LORA_GNSS_TX_PIN);
  loraGnssStarted = true;
  loraGnssStatus = "Listening 115200";
  Serial.printf("LoRa diag: GNSS UART started RX=G%d TX=G%d baud=%lu.\n",
                LORA_GNSS_RX_PIN, LORA_GNSS_TX_PIN,
                static_cast<unsigned long>(LORA_GNSS_BAUD));
}

void serviceLoraGnssSerial() {
  if (!loraGnssStarted) {
    return;
  }

  while (loraGnssSerial.available() > 0) {
    const char ch = static_cast<char>(loraGnssSerial.read());
    loraGnssByteCount++;

    if (ch == '\r') {
      continue;
    }

    if (ch == '\n') {
      loraGnssLineBuffer.trim();
      if (loraGnssLineBuffer.length() > 0) {
        loraLastNmeaLine = loraGnssLineBuffer;
        loraGnssLineCount++;
        loraGnssStatus = "NMEA received";
      }
      loraGnssLineBuffer = "";
      continue;
    }

    if (loraGnssLineBuffer.length() < LORA_GNSS_MAX_LINE_CHARS) {
      loraGnssLineBuffer += ch;
    }
  }

  if (loraGnssByteCount == 0) {
    loraGnssStatus = "Waiting NMEA";
  }
}

void updateLoraRadioReceive() {
  if (!loraRadioReady) {
    return;
  }

  if (!loraPacketReceived) {
    return;
  }

  loraPacketReceived = false;

  String packet;
  const int state = loraRadio.readData(packet);
  loraRadioState = state;

  if (state == RADIOLIB_ERR_NONE) {
    loraPacketCount++;
    loraLastPacket = clippedLoraText(packet, 28);
    loraLastRssi = loraRadio.getRSSI();
    loraLastSnr = loraRadio.getSNR();
    loraRadioStatus = "Packet received";
    Serial.printf("LoRa diag: packet #%lu RSSI=%.1f SNR=%.1f data=%s\n",
                  static_cast<unsigned long>(loraPacketCount), loraLastRssi,
                  loraLastSnr, loraLastPacket.c_str());
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    loraCrcErrorCount++;
    loraRadioStatus = "CRC mismatch";
    Serial.println("LoRa diag: CRC mismatch.");
  } else {
    loraReceiveErrorCount++;
    loraRadioStatus = String("RX error ") + state;
    Serial.printf("LoRa diag: receive failed code=%d.\n", state);
  }

  const int listenState = loraRadio.startReceive();
  if (listenState == RADIOLIB_ERR_NONE) {
    loraListening = true;
  } else {
    loraListening = false;
    loraRadioStatus = String("Listen failed ") + listenState;
    Serial.printf("LoRa diag: listen restart failed code=%d.\n", listenState);
  }
}

}  // namespace

void showLoraDiag() {
  if (!loraDiagInitialized) {
    initLoraDiagnostics();
  }

  serviceLoraDiagnostics();
  renderLoraDiag();
}

void renderLoraDiag() {
  lastLoraDiagRenderMs = millis();
  beginContentDraw();

  contentCanvas.printf("Cap:%s RF:%s\n",
                       loraIoExpanderDetected ? "yes" : "no",
                       loraRfSwitchEnabled ? "on" : "off");
  contentCanvas.printf("Radio:%s\n",
                       clippedLoraText(loraRadioStatus, 23).c_str());
  contentCanvas.printf("RX %.1fMHz Pk:%lu\n", LORA_DIAG_RX_FREQUENCY_MHZ,
                       static_cast<unsigned long>(loraPacketCount));
  contentCanvas.printf("RSSI:%s SNR:%s\n",
                       loraMetricText(loraLastRssi, loraPacketCount).c_str(),
                       loraMetricText(loraLastSnr, loraPacketCount).c_str());
  contentCanvas.printf("GNSS:%luB %luL\n",
                       static_cast<unsigned long>(loraGnssByteCount),
                       static_cast<unsigned long>(loraGnssLineCount));
  contentCanvas.printf("NMEA:%s\n",
                       clippedLoraText(loraLastNmeaLine.length() > 0
                                           ? loraLastNmeaLine
                                           : loraGnssStatus,
                                       23)
                           .c_str());
  contentCanvas.println("OK/R retry Back");
  contentCanvas.println(LORA_DIAG_NO_TX_NOTICE);

  commitContentDraw();
}

bool initLoraDiagnostics() {
  resetLoraDiagnostics();
  loraDiagInitialized = true;
  loraStatus = "Starting.";
  loraRadioStatus = "Starting.";

  Serial.println("LoRa diag: starting Cap LoRa-1262 diagnostics.");
  Serial.println("LoRa diag: RX-only. No TX path is enabled.");
  prepareSharedExtSpiForLora();
  startLoraGnssSerial();

  const bool internalI2cReady =
      m5::In_I2C.isEnabled() || m5::In_I2C.begin();
  if (!internalI2cReady) {
    loraStatus = "Internal I2C failed";
    loraRadioStatus = "I2C failed";
    Serial.println("LoRa diag: internal I2C init failed.");
    return false;
  }

  if (!loraIoExpander.begin()) {
    loraStatus = "LoRa not found";
    loraRadioStatus = "I/O expander missing";
    Serial.printf("LoRa diag: PI4IOE5V6408 0x%02X not found.\n",
                  LORA_IO_EXPANDER_ADDRESS);
    return false;
  }

  loraIoExpanderDetected = true;
  loraIoExpander.setDirection(LORA_RF_SWITCH_PIN, true);
  loraIoExpander.setHighImpedance(LORA_RF_SWITCH_PIN, false);
  loraIoExpander.digitalWrite(LORA_RF_SWITCH_PIN, true);
  loraRfSwitchEnabled = true;

  Serial.printf("LoRa diag: PI4IOE5V6408 found at 0x%02X; P%u RF switch high.\n",
                LORA_IO_EXPANDER_ADDRESS, LORA_RF_SWITCH_PIN);

  loraRadioState =
      loraRadio.begin(LORA_DIAG_RX_FREQUENCY_MHZ, LORA_DIAG_BANDWIDTH_KHZ,
                      LORA_DIAG_SPREADING_FACTOR, LORA_DIAG_CODING_RATE,
                      LORA_DIAG_SYNC_WORD, LORA_DIAG_UNUSED_TX_POWER_DBM,
                      LORA_DIAG_PREAMBLE_LEN, 3.0, true);

  if (loraRadioState != RADIOLIB_ERR_NONE) {
    loraStatus = "LoRa not found";
    loraRadioStatus = String("Init failed ") + loraRadioState;
    Serial.printf("LoRa diag: SX1262 init failed code=%d.\n", loraRadioState);
    return false;
  }

  loraRadioReady = true;
  loraRadio.setCurrentLimit(140);
  loraRadio.setPacketReceivedAction(setLoraPacketReceivedFlag);

  loraRadioState = loraRadio.startReceive();
  if (loraRadioState != RADIOLIB_ERR_NONE) {
    loraListening = false;
    loraRadioStatus = String("Listen failed ") + loraRadioState;
    Serial.printf("LoRa diag: startReceive failed code=%d.\n", loraRadioState);
    return false;
  }

  loraListening = true;
  loraStatus = "Listening.";
  loraRadioStatus = "Listening RX only";
  Serial.printf("LoRa diag: SX1262 listening at %.1f MHz, BW %.1f kHz, SF%u.\n",
                LORA_DIAG_RX_FREQUENCY_MHZ, LORA_DIAG_BANDWIDTH_KHZ,
                LORA_DIAG_SPREADING_FACTOR);
  return true;
}

void serviceLoraDiagnostics() {
  if (!loraDiagInitialized) {
    return;
  }

  const unsigned long now = millis();
  if (now - lastLoraDiagServiceMs < LORA_DIAG_SERVICE_INTERVAL_MS) {
    return;
  }

  lastLoraDiagServiceMs = now;
  serviceLoraGnssSerial();
  updateLoraRadioReceive();

  if (currentScreen == Screen::LoraDiag &&
      now - lastLoraDiagRenderMs >= LORA_DIAG_RENDER_INTERVAL_MS) {
    renderLoraDiag();
  }
}

void resetLoraDiagnostics() {
  loraDiagInitialized = false;
  loraIoExpanderDetected = false;
  loraRfSwitchEnabled = false;
  loraRadioReady = false;
  loraListening = false;
  loraPacketReceived = false;
  loraStatus = "Not initialized.";
  loraRadioStatus = "Not initialized.";
  loraGnssStatus = "Not started.";
  loraLastPacket = "";
  loraLastNmeaLine = "";
  loraGnssLineBuffer = "";
  loraRadioState = 0;
  loraLastRssi = 0.0f;
  loraLastSnr = 0.0f;
  loraPacketCount = 0;
  loraCrcErrorCount = 0;
  loraReceiveErrorCount = 0;
  loraGnssByteCount = 0;
  loraGnssLineCount = 0;
  lastLoraDiagServiceMs = 0;
  lastLoraDiagRenderMs = 0;
}

void stopLoraDiagnostics() {
  if (loraRadioReady) {
    loraRadio.sleep();
  }

  if (loraGnssStarted) {
    loraGnssSerial.end();
  }

  resetLoraDiagnostics();
  loraGnssStarted = false;
}
