#include "app.h"

void deselectSharedSpiDevices() {
  pinMode(SD_SPI_CS_PIN, OUTPUT);
  digitalWrite(SD_SPI_CS_PIN, HIGH);
  pinMode(LORA_NSS_PIN, OUTPUT);
  digitalWrite(LORA_NSS_PIN, HIGH);
  delayMicroseconds(100);
}

void prepareSharedSpiForSd() {
  if (sharedSpiOwner == SHARED_SPI_OWNER_SD) {
    deselectSharedSpiDevices();
    return;
  }

  deselectSharedSpiDevices();
  SPI.end();
  delay(2);
  deselectSharedSpiDevices();
  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
  deselectSharedSpiDevices();
  sharedSpiOwner = SHARED_SPI_OWNER_SD;
}
