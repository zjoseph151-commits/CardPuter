#include "app.h"

#include <RadioLib.h>
#include "utility/PI4IOE5V6408_Class.hpp"

namespace {

m5::PI4IOE5V6408_Class loraIoExpander(LORA_IO_EXPANDER_ADDRESS, 400000,
                                       &m5::In_I2C);
SX1262 loraRadio = new Module(LORA_NSS_PIN, LORA_IRQ_PIN, LORA_RST_PIN,
                               LORA_BUSY_PIN);
volatile bool loraPacketReceived = false;
bool loraHasInstantRssi = false;
float loraInstantRssi = 0.0f;

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

String loraInstantRssiText() {
  if (!loraHasInstantRssi) {
    return "--";
  }

  char metric[12];
  snprintf(metric, sizeof(metric), "%.1f", loraInstantRssi);
  return metric;
}

String loraSnrText() {
  if (loraPacketCount == 0) {
    return "--pkt";
  }

  return loraMetricText(loraLastSnr, loraPacketCount);
}

void prepareSharedExtSpiForLora() {
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

void updateLoraRadioReceive() {
  if (!loraRadioReady) {
    return;
  }

  if (loraListening) {
    loraInstantRssi = loraRadio.getRSSI(false);
    loraHasInstantRssi = isfinite(loraInstantRssi);
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
                       (loraPacketCount > 0
                            ? loraMetricText(loraLastRssi, loraPacketCount)
                            : loraInstantRssiText())
                           .c_str(),
                       loraSnrText().c_str());
  contentCanvas.printf("GNSS:%s S:%s HD:%s\n",
                       loraGnssHasFreshFix() ? "Fix" : "NoFix",
                       loraGnssSatellitesText().c_str(),
                       loraGnssHdopText().c_str());
  contentCanvas.println(clippedLoraText(
      loraGnssCoordinateText("Lat", loraGnssLatitude), 24));
  contentCanvas.println(clippedLoraText(
      loraGnssCoordinateText("Lon", loraGnssLongitude), 24));
  contentCanvas.println(clippedLoraText(loraGnssTimeText(), 24));
  contentCanvas.printf("NMEA:%lu/%lu bad:%lu\n",
                       static_cast<unsigned long>(loraGnssLineCount),
                       static_cast<unsigned long>(loraGnssPassedChecksum),
                       static_cast<unsigned long>(loraGnssFailedChecksum));
  contentCanvas.printf("OK/R Back %s\n", LORA_DIAG_NO_TX_NOTICE);

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
  loraLastPacket = "";
  loraRadioState = 0;
  loraLastRssi = 0.0f;
  loraLastSnr = 0.0f;
  loraInstantRssi = 0.0f;
  loraHasInstantRssi = false;
  loraPacketCount = 0;
  loraCrcErrorCount = 0;
  loraReceiveErrorCount = 0;
  resetLoraGnssParser();
  lastLoraDiagServiceMs = 0;
  lastLoraDiagRenderMs = 0;
}

void stopLoraDiagnostics() {
  if (loraRadioReady) {
    loraRadio.sleep();
  }

  deselectSharedSpiDevices();
  stopLoraGnssSerial();

  resetLoraDiagnostics();
}
