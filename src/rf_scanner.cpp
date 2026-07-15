#include "app.h"

void showRfScanner() {
  if (!rfScanHasData && !rfScanInProgress) {
    scanRfChannels();
    return;
  }

  renderRfScanner();
}

bool initNrf24Radio() {
  nrf24Initialized = false;
  nrf24ChipConnected = false;
  nrf24Listening = false;
  nrf24Status = "Initializing...";

  pinMode(SD_SPI_CS_PIN, OUTPUT);
  digitalWrite(SD_SPI_CS_PIN, HIGH);
  pinMode(NRF24_CSN_PIN, OUTPUT);
  digitalWrite(NRF24_CSN_PIN, HIGH);
  pinMode(NRF24_CE_PIN, OUTPUT);
  digitalWrite(NRF24_CE_PIN, LOW);

  SPI.begin(NRF24_SPI_SCK_PIN, NRF24_SPI_MISO_PIN, NRF24_SPI_MOSI_PIN,
            NRF24_CSN_PIN);
  delay(5);

  if (!nrf24Radio.begin(&SPI, NRF24_CE_PIN, NRF24_CSN_PIN)) {
    nrf24Status = "Begin failed.";
    rfScanStatus = "Radio begin failed.";
    Serial.println("NRF24: radio hardware not responding.");
    return false;
  }

  if (!nrf24Radio.isChipConnected()) {
    nrf24Status = "No radio on SPI.";
    rfScanStatus = "No radio on SPI.";
    Serial.println("NRF24: chip not detected on SPI.");
    return false;
  }

  nrf24Radio.setAddressWidth(5);
  nrf24Radio.setChannel(rfScanSelectedChannel);
  nrf24Radio.setPayloadSize(NRF24_PAYLOAD_SIZE);
  nrf24Radio.setDataRate(NRF24_DATA_RATE);
  nrf24Radio.setCRCLength(RF24_CRC_16);
  nrf24Radio.disableDynamicPayloads();
  nrf24Radio.setPALevel(RF24_PA_LOW);
  nrf24Radio.setAutoAck(false);
  nrf24Radio.setRetries(0, 0);
  nrf24Radio.openReadingPipe(NRF24_RX_PIPE, NRF24_SHARED_ADDRESS);
  nrf24Radio.flush_tx();
  nrf24Radio.flush_rx();
  nrf24Radio.clearStatusFlags(RF24_IRQ_ALL);
  resumeNrf24Listening();

  nrf24Initialized = true;
  nrf24ChipConnected = true;
  nrf24Status = "Radio ready.";
  rfScanStatus = "Radio ready.";
  Serial.println("NRF24: ready for RF channel scanning.");
  return true;
}

void powerDownNrf24Radio() {
  if (!nrf24Initialized) {
    return;
  }

  nrf24Radio.powerDown();
  nrf24Listening = false;
  nrf24Status = "Powered down.";
}

void resumeNrf24Listening() {
  nrf24Radio.startListening();
  nrf24Radio.closeReadingPipe(0);
  nrf24Listening = true;
}

bool scanRfChannels() {
  if (!nrf24Initialized || !nrf24ChipConnected) {
    if (!initNrf24Radio()) {
      rfScanHasData = false;
      rfScanInProgress = false;
      renderRfScanner();
      return false;
    }
  }

  rfScanHasData = false;
  rfScanInProgress = true;
  rfScanStatus = "Scanning 0-125...";
  renderRfScanner();

  for (uint8_t channel = 0; channel < RF_SCAN_CHANNEL_COUNT; ++channel) {
    rfScanStatus = "Scanning ch " + String(channel);
    rfScanSelectedChannel = channel;

    nrf24Radio.setChannel(channel);
    nrf24Radio.flush_rx();
    nrf24Radio.clearStatusFlags(RF24_IRQ_ALL);
    resumeNrf24Listening();
    delayMicroseconds(150);

    uint8_t activity = 0;
    for (uint8_t sample = 0; sample < RF_SCAN_SAMPLE_COUNT; ++sample) {
      delay(RF_SCAN_DWELL_MS);
      if (nrf24Radio.testRPD() || nrf24Radio.testCarrier()) {
        activity++;
      }
    }

    rfScanActivity[channel] = activity;

    if (channel % 21 == 0) {
      renderRfScanner();
    }
    yield();
  }

  rfScanInProgress = false;
  rfScanHasData = true;
  rfScanCompletedAtMs = millis();
  updateQuietRfChannels();
  if (rfScanQuietChannels[0] < RF_SCAN_CHANNEL_COUNT) {
    rfScanSelectedChannel = rfScanQuietChannels[0];
  }
  nrf24Radio.setChannel(rfScanSelectedChannel);
  resumeNrf24Listening();
  rfScanStatus = "Scan complete.";
  renderRfScanner();
  return true;
}

void renderRfScanner() {
  beginContentDraw();

  const bool ready = nrf24Initialized && nrf24ChipConnected;
  if (ready) {
    contentCanvas.printf("Radio:Ready %s CH:%u\n",
                         nrf24DataRateText(nrf24Radio.getDataRate()),
                         nrf24Radio.getChannel());
  } else {
    contentCanvas.println("Radio:Not found");
  }

  if (!ready) {
    contentCanvas.println(rfScanStatus.substring(0, 24));
    contentCanvas.println("Check wiring/power.");
    contentCanvas.println("R retry");
    commitContentDraw();
    return;
  }

  if (rfScanHasData) {
    contentCanvas.print("Quiet:");
    for (uint8_t i = 0; i < RF_SCAN_QUIET_COUNT; ++i) {
      if (rfScanQuietChannels[i] < RF_SCAN_CHANNEL_COUNT) {
        contentCanvas.printf(" %u", rfScanQuietChannels[i]);
      }
    }
    contentCanvas.println();
  } else {
    contentCanvas.println(rfScanStatus.substring(0, 24));
  }

  drawRfScanGraph();

  contentCanvas.setCursor(8, 84);
  contentCanvas.print("0   25  50  75 100 125");
  contentCanvas.setCursor(8, 96);
  if (rfScanHasData) {
    contentCanvas.printf("CH:%u Act:%u/%u  R scan",
                         rfScanSelectedChannel,
                         rfScanActivity[rfScanSelectedChannel],
                         RF_SCAN_SAMPLE_COUNT);
  } else {
    contentCanvas.print("R scan  ,/. select");
  }

  commitContentDraw();
}

void drawRfScanGraph() {
  const int graphX = 8;
  const int graphY = 34;
  const int graphW = contentCanvas.width() - 16;
  const int graphH = 44;

  contentCanvas.drawRect(graphX - 1, graphY - 1, graphW + 2, graphH + 2,
                         DARKGREY);

  for (uint8_t channel = 0; channel < RF_SCAN_CHANNEL_COUNT; ++channel) {
    const int x0 = graphX + (static_cast<int>(channel) * graphW) /
                             RF_SCAN_CHANNEL_COUNT;
    const int x1 = graphX + (static_cast<int>(channel + 1) * graphW) /
                             RF_SCAN_CHANNEL_COUNT;
    const int barW = max(1, x1 - x0);
    const uint8_t activity = rfScanActivity[channel];
    const int barH =
        activity > 0 ? max(2, (static_cast<int>(activity) * graphH) /
                                  RF_SCAN_SAMPLE_COUNT)
                     : 1;
    const int y = graphY + graphH - barH;
    uint16_t color = DARKGREY;

    if (isQuietRfChannel(channel)) {
      color = GREEN;
    } else if (activity >= RF_SCAN_SAMPLE_COUNT - 1) {
      color = RED;
    } else if (activity >= 2) {
      color = ORANGE;
    } else if (activity > 0) {
      color = YELLOW;
    }

    contentCanvas.fillRect(x0, y, barW, barH, color);
  }

  const int selectedX = graphX + (static_cast<int>(rfScanSelectedChannel) *
                                 graphW) /
                                    RF_SCAN_CHANNEL_COUNT;
  contentCanvas.drawFastVLine(selectedX, graphY, graphH, WHITE);
}

void updateQuietRfChannels() {
  for (uint8_t i = 0; i < RF_SCAN_QUIET_COUNT; ++i) {
    rfScanQuietChannels[i] = 0xFF;
  }

  for (uint8_t rank = 0; rank < RF_SCAN_QUIET_COUNT; ++rank) {
    uint8_t bestChannel = 0xFF;
    uint8_t bestActivity = UINT8_MAX;

    for (uint8_t pass = 0; pass < 2 && bestChannel >= RF_SCAN_CHANNEL_COUNT;
         ++pass) {
      const bool enforceSpacing = pass == 0;
      for (uint8_t channel = 0; channel < RF_SCAN_CHANNEL_COUNT; ++channel) {
        bool alreadyChosen = false;
        bool tooClose = false;

        for (uint8_t i = 0; i < rank; ++i) {
          if (rfScanQuietChannels[i] == channel) {
            alreadyChosen = true;
            break;
          }
          if (rfScanQuietChannels[i] < RF_SCAN_CHANNEL_COUNT &&
              abs(static_cast<int>(channel) -
                  static_cast<int>(rfScanQuietChannels[i])) <
                  RF_SCAN_MIN_QUIET_SPACING) {
            tooClose = true;
          }
        }

        if (alreadyChosen || (enforceSpacing && tooClose)) {
          continue;
        }

        const uint8_t activity = rfScanActivity[channel];
        if (activity < bestActivity) {
          bestActivity = activity;
          bestChannel = channel;
        }
      }
    }

    rfScanQuietChannels[rank] = bestChannel;
  }
}

bool isQuietRfChannel(uint8_t channel) {
  if (!rfScanHasData) {
    return false;
  }

  for (uint8_t i = 0; i < RF_SCAN_QUIET_COUNT; ++i) {
    if (rfScanQuietChannels[i] == channel) {
      return true;
    }
  }

  return false;
}

void moveRfScanSelection(int direction) {
  int nextChannel = static_cast<int>(rfScanSelectedChannel) + direction;
  if (nextChannel < 0) {
    nextChannel = RF_SCAN_CHANNEL_COUNT - 1;
  } else if (nextChannel >= RF_SCAN_CHANNEL_COUNT) {
    nextChannel = 0;
  }

  rfScanSelectedChannel = static_cast<uint8_t>(nextChannel);
  if (nrf24Initialized && nrf24ChipConnected) {
    nrf24Radio.setChannel(rfScanSelectedChannel);
    resumeNrf24Listening();
  }
  renderRfScanner();
}

const char* nrf24DataRateText(rf24_datarate_e dataRate) {
  switch (dataRate) {
    case RF24_250KBPS:
      return "250K";
    case RF24_2MBPS:
      return "2M";
    case RF24_1MBPS:
    default:
      return "1M";
  }
}
