#include "app.h"

#include <RadioLib.h>
#include "utility/PI4IOE5V6408_Class.hpp"

namespace {

m5::PI4IOE5V6408_Class packetMonitorIoExpander(LORA_IO_EXPANDER_ADDRESS,
                                                400000, &m5::In_I2C);
SX1262 packetMonitorRadio = new Module(LORA_NSS_PIN, LORA_IRQ_PIN,
                                        LORA_RST_PIN, LORA_BUSY_PIN);
volatile bool packetMonitorReceived = false;

#if defined(ESP32)
void IRAM_ATTR setPacketMonitorReceivedFlag() {
#else
void setPacketMonitorReceivedFlag() {
#endif
  packetMonitorReceived = true;
}

String clippedPacketText(const String& text, int maxChars) {
  return text.substring(0, maxChars);
}

String packetMetricText(float value, uint32_t count) {
  if (count == 0) {
    return "--";
  }

  char metric[12];
  snprintf(metric, sizeof(metric), "%.1f", value);
  return metric;
}

String packetInstantRssiText() {
  if (!loraPacketMonitorHasInstantRssi) {
    return "--";
  }

  char metric[12];
  snprintf(metric, sizeof(metric), "%.1f", loraPacketMonitorInstantRssi);
  return metric;
}

String packetSnrText() {
  if (loraPacketMonitorPacketCount == 0) {
    return "--pkt";
  }

  return packetMetricText(loraPacketMonitorLastSnr,
                          loraPacketMonitorPacketCount);
}

String packetAgeText() {
  if (loraPacketMonitorLastPacketMs == 0) {
    return "--";
  }

  return String((millis() - loraPacketMonitorLastPacketMs) / 1000UL) + "s";
}

String sanitizePacketPayload(const String& payload) {
  String clean;

  for (int i = 0; i < payload.length() &&
                  clean.length() < LORA_PACKET_MONITOR_PAYLOAD_MAX_CHARS;
       ++i) {
    const char c = payload.charAt(i);
    if (c == '\r' || c == '\n' || c == '\t') {
      clean += ' ';
    } else if (c >= 32 && c <= 126) {
      clean += c;
    } else {
      clean += '.';
    }
  }

  if (payload.length() > LORA_PACKET_MONITOR_PAYLOAD_MAX_CHARS &&
      clean.length() > 0) {
    clean.setCharAt(clean.length() - 1, '~');
  }

  return clean;
}

void prepareSharedExtSpiForPacketMonitor() {
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

void updatePacketMonitorReceive() {
  if (!loraPacketMonitorRadioReady) {
    return;
  }

  if (loraPacketMonitorListening) {
    loraPacketMonitorInstantRssi = packetMonitorRadio.getRSSI(false);
    loraPacketMonitorHasInstantRssi = isfinite(loraPacketMonitorInstantRssi);
  }

  if (!packetMonitorReceived) {
    return;
  }

  packetMonitorReceived = false;

  String packet;
  const int state = packetMonitorRadio.readData(packet);
  loraPacketMonitorRadioState = state;

  if (state == RADIOLIB_ERR_NONE) {
    loraPacketMonitorPacketCount++;
    loraPacketMonitorLastPacketLength = packet.length();
    loraPacketMonitorLastPayload = sanitizePacketPayload(packet);
    loraPacketMonitorLastRssi = packetMonitorRadio.getRSSI();
    loraPacketMonitorLastSnr = packetMonitorRadio.getSNR();
    loraPacketMonitorLastPacketMs = millis();
    loraPacketMonitorStatus = "Packet received";
    Serial.printf("LoRa Packet Monitor: packet #%lu len=%u RSSI=%.1f SNR=%.1f\n",
                  static_cast<unsigned long>(loraPacketMonitorPacketCount),
                  loraPacketMonitorLastPacketLength,
                  loraPacketMonitorLastRssi, loraPacketMonitorLastSnr);
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    loraPacketMonitorCrcErrorCount++;
    loraPacketMonitorStatus = "CRC mismatch";
    Serial.println("LoRa Packet Monitor: CRC mismatch.");
  } else {
    loraPacketMonitorReceiveErrorCount++;
    loraPacketMonitorStatus = String("RX error ") + state;
    Serial.printf("LoRa Packet Monitor: receive failed code=%d.\n", state);
  }

  const int listenState = packetMonitorRadio.startReceive();
  if (listenState == RADIOLIB_ERR_NONE) {
    loraPacketMonitorListening = true;
  } else {
    loraPacketMonitorListening = false;
    loraPacketMonitorStatus = String("Listen failed ") + listenState;
    Serial.printf("LoRa Packet Monitor: listen restart failed code=%d.\n",
                  listenState);
  }
}

}  // namespace

void showLoraPacketMonitor() {
  if (!loraPacketMonitorInitialized) {
    initLoraPacketMonitor();
  }

  serviceLoraPacketMonitor();
  renderLoraPacketMonitor();
}

void renderLoraPacketMonitor() {
  lastLoraPacketMonitorRenderMs = millis();
  beginContentDraw();

  contentCanvas.printf("RX %.1fMHz BW%.0f SF%u\n", LORA_DIAG_RX_FREQUENCY_MHZ,
                       LORA_DIAG_BANDWIDTH_KHZ,
                       LORA_DIAG_SPREADING_FACTOR);
  contentCanvas.printf("Radio:%s\n",
                       clippedPacketText(loraPacketMonitorStatus, 23).c_str());
  contentCanvas.printf("Pk:%lu CRC:%lu Err:%lu\n",
                       static_cast<unsigned long>(loraPacketMonitorPacketCount),
                       static_cast<unsigned long>(
                           loraPacketMonitorCrcErrorCount),
                       static_cast<unsigned long>(
                           loraPacketMonitorReceiveErrorCount));
  const bool hasPacket = loraPacketMonitorPacketCount > 0;
  contentCanvas.printf("%s:%sdBm SNR:%s\n", hasPacket ? "PktRSSI" : "Noise",
                       (hasPacket ? packetMetricText(loraPacketMonitorLastRssi,
                                                     loraPacketMonitorPacketCount)
                                  : packetInstantRssiText())
                           .c_str(),
                       packetSnrText().c_str());
  contentCanvas.printf("Len:%u Age:%s\n",
                       loraPacketMonitorLastPacketLength,
                       packetAgeText().c_str());
  contentCanvas.println("Payload:");
  contentCanvas.println(clippedPacketText(
      loraPacketMonitorLastPayload.length() > 0
          ? loraPacketMonitorLastPayload
          : String("(need matching TX)"),
      28));
  contentCanvas.println("OK/R restart C clear");
  contentCanvas.println(LORA_PACKET_MONITOR_NO_TX_NOTICE);

  commitContentDraw();
}

bool initLoraPacketMonitor() {
  resetLoraPacketMonitor();
  loraPacketMonitorInitialized = true;
  loraPacketMonitorStatus = "Starting.";

  Serial.println("LoRa Packet Monitor: starting RX-only monitor.");
  Serial.println("LoRa Packet Monitor: RX only. No TX.");
  prepareSharedExtSpiForPacketMonitor();

  const bool internalI2cReady =
      m5::In_I2C.isEnabled() || m5::In_I2C.begin();
  if (!internalI2cReady) {
    loraPacketMonitorStatus = "I2C failed";
    Serial.println("LoRa Packet Monitor: internal I2C init failed.");
    return false;
  }

  if (!packetMonitorIoExpander.begin()) {
    loraPacketMonitorStatus = "LoRa not found";
    Serial.printf("LoRa Packet Monitor: PI4IOE5V6408 0x%02X not found.\n",
                  LORA_IO_EXPANDER_ADDRESS);
    return false;
  }

  loraPacketMonitorIoExpanderDetected = true;
  packetMonitorIoExpander.setDirection(LORA_RF_SWITCH_PIN, true);
  packetMonitorIoExpander.setHighImpedance(LORA_RF_SWITCH_PIN, false);
  packetMonitorIoExpander.digitalWrite(LORA_RF_SWITCH_PIN, true);
  loraPacketMonitorRfSwitchEnabled = true;

  loraPacketMonitorRadioState =
      packetMonitorRadio.begin(LORA_DIAG_RX_FREQUENCY_MHZ,
                               LORA_DIAG_BANDWIDTH_KHZ,
                               LORA_DIAG_SPREADING_FACTOR,
                               LORA_DIAG_CODING_RATE, LORA_DIAG_SYNC_WORD,
                               LORA_DIAG_UNUSED_TX_POWER_DBM,
                               LORA_DIAG_PREAMBLE_LEN, 3.0, true);

  if (loraPacketMonitorRadioState != RADIOLIB_ERR_NONE) {
    loraPacketMonitorStatus =
        String("Init failed ") + loraPacketMonitorRadioState;
    Serial.printf("LoRa Packet Monitor: SX1262 init failed code=%d.\n",
                  loraPacketMonitorRadioState);
    return false;
  }

  loraPacketMonitorRadioReady = true;
  packetMonitorRadio.setCurrentLimit(140);
  packetMonitorRadio.setPacketReceivedAction(setPacketMonitorReceivedFlag);

  loraPacketMonitorRadioState = packetMonitorRadio.startReceive();
  if (loraPacketMonitorRadioState != RADIOLIB_ERR_NONE) {
    loraPacketMonitorListening = false;
    loraPacketMonitorStatus =
        String("Listen failed ") + loraPacketMonitorRadioState;
    Serial.printf("LoRa Packet Monitor: startReceive failed code=%d.\n",
                  loraPacketMonitorRadioState);
    return false;
  }

  loraPacketMonitorListening = true;
  loraPacketMonitorStatus = "No packets yet";
  Serial.printf("LoRa Packet Monitor: listening at %.1f MHz, BW %.1f kHz, SF%u.\n",
                LORA_DIAG_RX_FREQUENCY_MHZ, LORA_DIAG_BANDWIDTH_KHZ,
                LORA_DIAG_SPREADING_FACTOR);
  return true;
}

void serviceLoraPacketMonitor() {
  if (!loraPacketMonitorInitialized) {
    return;
  }

  const unsigned long now = millis();
  if (now - lastLoraPacketMonitorServiceMs <
      LORA_PACKET_MONITOR_SERVICE_INTERVAL_MS) {
    return;
  }

  lastLoraPacketMonitorServiceMs = now;
  updatePacketMonitorReceive();

  if (currentScreen == Screen::LoraPacketMonitor &&
      now - lastLoraPacketMonitorRenderMs >=
          LORA_PACKET_MONITOR_RENDER_INTERVAL_MS) {
    renderLoraPacketMonitor();
  }
}

void resetLoraPacketMonitor() {
  loraPacketMonitorInitialized = false;
  loraPacketMonitorIoExpanderDetected = false;
  loraPacketMonitorRfSwitchEnabled = false;
  loraPacketMonitorRadioReady = false;
  loraPacketMonitorListening = false;
  loraPacketMonitorHasInstantRssi = false;
  packetMonitorReceived = false;
  loraPacketMonitorStatus = "Not initialized.";
  loraPacketMonitorLastPayload = "";
  loraPacketMonitorRadioState = 0;
  loraPacketMonitorLastRssi = 0.0f;
  loraPacketMonitorLastSnr = 0.0f;
  loraPacketMonitorInstantRssi = 0.0f;
  loraPacketMonitorPacketCount = 0;
  loraPacketMonitorCrcErrorCount = 0;
  loraPacketMonitorReceiveErrorCount = 0;
  loraPacketMonitorLastPacketLength = 0;
  loraPacketMonitorLastPacketMs = 0;
  lastLoraPacketMonitorServiceMs = 0;
  lastLoraPacketMonitorRenderMs = 0;
}

void stopLoraPacketMonitor() {
  if (loraPacketMonitorRadioReady) {
    packetMonitorRadio.sleep();
  }

  deselectSharedSpiDevices();
  resetLoraPacketMonitor();
}

void clearLoraPacketMonitor() {
  loraPacketMonitorLastPayload = "";
  loraPacketMonitorPacketCount = 0;
  loraPacketMonitorCrcErrorCount = 0;
  loraPacketMonitorReceiveErrorCount = 0;
  loraPacketMonitorLastPacketLength = 0;
  loraPacketMonitorLastPacketMs = 0;
  loraPacketMonitorStatus = loraPacketMonitorListening ? "Cleared; no pkts"
                                                       : "Cleared.";
}
