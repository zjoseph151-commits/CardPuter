#include <Arduino.h>
#include <esp_sleep.h>
#include <RadioLib.h>
#include <SPI.h>

namespace {

constexpr const char* NODE_NAME = "XIAO SX1262 LoRa ACK Node";
constexpr const char* DEVICE_ID = "xiao-sx1262-ack";
constexpr float XIAO_LORA_FREQUENCY_MHZ = 915.0f;
constexpr float XIAO_LORA_BANDWIDTH_KHZ = 125.0f;
constexpr uint8_t XIAO_LORA_SPREADING_FACTOR = 12;
constexpr uint8_t XIAO_LORA_CODING_RATE = 5;
constexpr uint8_t XIAO_LORA_SYNC_WORD = 0x34;
constexpr uint16_t XIAO_LORA_PREAMBLE_LEN = 20;
constexpr int8_t XIAO_LORA_TX_POWER_DBM = 5;
constexpr float XIAO_LORA_CURRENT_LIMIT_MA = 60.0f;
constexpr float XIAO_LORA_TCXO_VOLTAGE = 3.0f;
constexpr bool XIAO_LORA_USE_REGULATOR_LDO = false;
constexpr uint32_t XIAO_LORA_SEND_COOLDOWN_MS = 10000;
constexpr uint32_t XIAO_LORA_ACK_COOLDOWN_MS = 1000;
constexpr uint32_t XIAO_LORA_STATUS_INTERVAL_MS = 5000;
constexpr uint32_t XIAO_LORA_BUTTON_DEBOUNCE_MS = 40;
constexpr uint64_t XIAO_LORA_DEEP_SLEEP_INTERVAL_US = 60ULL * 1000000ULL;

// Defaults target the standalone Wio-SX1262 for XIAO through-header pinout.
#ifndef XIAO_LORA_NSS_PIN
#define XIAO_LORA_NSS_PIN 5
#endif
#ifndef XIAO_LORA_DIO1_PIN
#define XIAO_LORA_DIO1_PIN 2
#endif
#ifndef XIAO_LORA_RST_PIN
#define XIAO_LORA_RST_PIN 3
#endif
#ifndef XIAO_LORA_BUSY_PIN
#define XIAO_LORA_BUSY_PIN 4
#endif
#ifndef XIAO_LORA_RF_SWITCH_PIN
#define XIAO_LORA_RF_SWITCH_PIN 1
#endif
#ifndef XIAO_LORA_USER_BUTTON_PIN
#define XIAO_LORA_USER_BUTTON_PIN 21
#endif
#ifndef XIAO_LORA_SPI_SCK_PIN
#define XIAO_LORA_SPI_SCK_PIN SCK
#endif
#ifndef XIAO_LORA_SPI_MISO_PIN
#define XIAO_LORA_SPI_MISO_PIN MISO
#endif
#ifndef XIAO_LORA_SPI_MOSI_PIN
#define XIAO_LORA_SPI_MOSI_PIN MOSI
#endif

SX1262 radio = new Module(XIAO_LORA_NSS_PIN, XIAO_LORA_DIO1_PIN,
                          XIAO_LORA_RST_PIN, XIAO_LORA_BUSY_PIN);

volatile bool packetReceived = false;

bool radioReady = false;
bool listening = false;
bool buttonHeld = false;
int lastButtonLevel = HIGH;
int lastRadioState = RADIOLIB_ERR_NONE;
uint32_t lastButtonChangeMs = 0;
uint32_t lastManualSendMs = 0;
uint32_t lastAckSendMs = 0;
uint32_t lastStatusMs = 0;
RTC_DATA_ATTR uint32_t manualSequence = 0;
uint32_t rxCount = 0;
uint32_t crcErrorCount = 0;
uint32_t rxErrorCount = 0;
uint32_t txCount = 0;
uint32_t txFailCount = 0;
uint32_t ackCount = 0;
uint32_t ackRateLimitedCount = 0;
float lastRssi = 0.0f;
float lastSnr = 0.0f;
String lastRxPayload = "none";
String lastTxPayload = "none";

#if defined(ESP32)
void IRAM_ATTR setPacketReceivedFlag() {
#else
void setPacketReceivedFlag() {
#endif
  packetReceived = true;
}

String sanitizePayload(const String& payload, size_t maxChars = 96) {
  String clean;
  for (int i = 0; i < payload.length() && clean.length() < maxChars; ++i) {
    const char c = payload.charAt(i);
    if (c == '\r' || c == '\n' || c == '\t') {
      clean += ' ';
    } else if (c >= 32 && c <= 126) {
      clean += c;
    } else {
      clean += '.';
    }
  }
  if (payload.length() > static_cast<int>(maxChars) && clean.length() > 0) {
    clean.setCharAt(clean.length() - 1, '~');
  }
  return clean;
}

String floatText(float value, uint8_t decimals = 1) {
  char text[18];
  snprintf(text, sizeof(text), "%.*f", decimals, value);
  return String(text);
}

const char* radioStateText(int state) {
  switch (state) {
    case RADIOLIB_ERR_NONE:
      return "ok";
    case RADIOLIB_ERR_CHIP_NOT_FOUND:
      return "chip-not-found";
    case RADIOLIB_ERR_CRC_MISMATCH:
      return "crc-mismatch";
    default:
      return "error";
  }
}

String csvField(const String& payload, uint8_t index) {
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
    current++;
  }
  return "";
}

void prepareReceiveMode() {
  digitalWrite(XIAO_LORA_RF_SWITCH_PIN, HIGH);
}

void prepareTransmitMode() {
  digitalWrite(XIAO_LORA_RF_SWITCH_PIN, LOW);
}

bool startListening() {
  if (!radioReady) {
    return false;
  }

  prepareReceiveMode();
  lastRadioState = radio.startReceive();
  listening = lastRadioState == RADIOLIB_ERR_NONE;
  if (!listening) {
    Serial.printf("Listen failed, code=%d\n", lastRadioState);
  }
  return listening;
}

bool sendPayload(const String& payload, bool enforceManualCooldown) {
  if (!radioReady) {
    Serial.println(F("TX skipped: radio not ready."));
    return false;
  }

  const uint32_t now = millis();
  if (enforceManualCooldown && lastManualSendMs != 0 &&
      now - lastManualSendMs < XIAO_LORA_SEND_COOLDOWN_MS) {
    const uint32_t remaining =
        (XIAO_LORA_SEND_COOLDOWN_MS - (now - lastManualSendMs) + 999) / 1000;
    Serial.printf("Manual probe rate-limited; wait %lu s.\n",
                  static_cast<unsigned long>(remaining));
    return false;
  }

  listening = false;
  packetReceived = false;
  prepareTransmitMode();
  String txPayload = payload;
  lastRadioState = radio.transmit(txPayload);
  prepareReceiveMode();
  packetReceived = false;

  if (lastRadioState == RADIOLIB_ERR_NONE) {
    txCount++;
    if (enforceManualCooldown) {
      lastManualSendMs = now;
    }
    lastTxPayload = sanitizePayload(payload);
    Serial.printf("TX ok #%lu: %s\n", static_cast<unsigned long>(txCount),
                  lastTxPayload.c_str());
  } else {
    txFailCount++;
    Serial.printf("TX failed #%lu, code=%d\n",
                  static_cast<unsigned long>(txFailCount), lastRadioState);
  }

  startListening();
  return lastRadioState == RADIOLIB_ERR_NONE;
}

void sendManualProbe() {
  const uint32_t nextSequence = manualSequence + 1;
  const String payload = String("SCBR,NODE,1,") + DEVICE_ID + "," +
                         String(nextSequence) + "," + String(millis());
  if (sendPayload(payload, true)) {
    manualSequence = nextSequence;
  }
}

void sendAckForPing(const String& pingPayload) {
  const uint32_t now = millis();
  if (lastAckSendMs != 0 && now - lastAckSendMs < XIAO_LORA_ACK_COOLDOWN_MS) {
    ackRateLimitedCount++;
    Serial.println(F("ACK skipped: cooldown active."));
    return;
  }

  String sequence = csvField(pingPayload, 4);
  if (sequence.length() == 0) {
    sequence = "0";
  }

  const String payload = String("SCBR,ACK,1,") + DEVICE_ID + "," + sequence +
                         "," + floatText(lastRssi) + "," +
                         floatText(lastSnr) + "," + String(now);
  lastAckSendMs = now;
  if (sendPayload(payload, false)) {
    ackCount++;
  }
}

void handlePacket(const String& payload) {
  if (payload.startsWith("SCBR,PING,1,")) {
    Serial.println(F("Ping matched; sending ACK."));
    sendAckForPing(payload);
  }
}

void serviceReceivedPacket() {
  if (!packetReceived) {
    return;
  }
  packetReceived = false;

  String payload;
  lastRadioState = radio.readData(payload);
  if (lastRadioState == RADIOLIB_ERR_NONE) {
    rxCount++;
    lastRssi = radio.getRSSI();
    lastSnr = radio.getSNR();
    lastRxPayload = sanitizePayload(payload);
    Serial.printf("RX #%lu RSSI=%s SNR=%s: %s\n",
                  static_cast<unsigned long>(rxCount),
                  floatText(lastRssi).c_str(), floatText(lastSnr).c_str(),
                  lastRxPayload.c_str());
    handlePacket(payload);
  } else if (lastRadioState == RADIOLIB_ERR_CRC_MISMATCH) {
    crcErrorCount++;
    Serial.println(F("RX CRC mismatch."));
  } else {
    rxErrorCount++;
    Serial.printf("RX failed, code=%d\n", lastRadioState);
  }

  startListening();
}

void printStatus() {
  Serial.printf(
      "Status radio=%s listen=%s state=%d rx=%lu crc=%lu err=%lu tx=%lu "
      "fail=%lu ack=%lu ack_rl=%lu state_text=%s freq=%.1f bw=%.0f sf=%u cr=4/%u sw=0x%02X "
      "pwr=%d\n",
      radioReady ? "ready" : "down", listening ? "yes" : "no", lastRadioState,
      static_cast<unsigned long>(rxCount),
      static_cast<unsigned long>(crcErrorCount),
      static_cast<unsigned long>(rxErrorCount),
      static_cast<unsigned long>(txCount),
      static_cast<unsigned long>(txFailCount),
      static_cast<unsigned long>(ackCount),
      static_cast<unsigned long>(ackRateLimitedCount),
      radioStateText(lastRadioState),
      XIAO_LORA_FREQUENCY_MHZ, XIAO_LORA_BANDWIDTH_KHZ,
      XIAO_LORA_SPREADING_FACTOR, XIAO_LORA_CODING_RATE,
      XIAO_LORA_SYNC_WORD, XIAO_LORA_TX_POWER_DBM);
  Serial.print(F("Last RX: "));
  Serial.println(lastRxPayload);
  Serial.print(F("Last TX: "));
  Serial.println(lastTxPayload);
}

bool initRadio();

void enterDeepSleep() {
  if (radioReady) {
    radio.clearDio1Action();
    radio.sleep();
  }

  SPI.end();
  digitalWrite(XIAO_LORA_RF_SWITCH_PIN, LOW);
  Serial.println(F("Sleeping for 60 seconds."));
  Serial.flush();
  esp_sleep_enable_timer_wakeup(XIAO_LORA_DEEP_SLEEP_INTERVAL_US);
  esp_deep_sleep_start();
}

void serviceSerial() {
  while (Serial.available() > 0) {
    const char key = static_cast<char>(Serial.read());
    if (key == 'p' || key == 'P') {
      sendManualProbe();
    } else if (key == 's' || key == 'S') {
      printStatus();
    } else if (key == 'r' || key == 'R') {
      Serial.println(F("Retrying SX1262 initialization..."));
      initRadio();
      printStatus();
    }
  }
}

void serviceButton() {
  const int level = digitalRead(XIAO_LORA_USER_BUTTON_PIN);
  const uint32_t now = millis();
  if (level != lastButtonLevel) {
    lastButtonLevel = level;
    lastButtonChangeMs = now;
  }

  if (now - lastButtonChangeMs < XIAO_LORA_BUTTON_DEBOUNCE_MS) {
    return;
  }

  if (level == LOW && !buttonHeld) {
    buttonHeld = true;
    sendManualProbe();
  } else if (level == HIGH) {
    buttonHeld = false;
  }
}

void serviceStatus() {
  const uint32_t now = millis();
  if (now - lastStatusMs < XIAO_LORA_STATUS_INTERVAL_MS) {
    return;
  }
  lastStatusMs = now;
  printStatus();
}

bool initRadio() {
  radioReady = false;
  listening = false;
  packetReceived = false;
  pinMode(XIAO_LORA_RF_SWITCH_PIN, OUTPUT);
  prepareReceiveMode();
  SPI.begin(XIAO_LORA_SPI_SCK_PIN, XIAO_LORA_SPI_MISO_PIN,
            XIAO_LORA_SPI_MOSI_PIN, XIAO_LORA_NSS_PIN);

  Serial.print(F("Initializing SX1262... "));
  lastRadioState =
      radio.begin(XIAO_LORA_FREQUENCY_MHZ, XIAO_LORA_BANDWIDTH_KHZ,
                  XIAO_LORA_SPREADING_FACTOR, XIAO_LORA_CODING_RATE,
                  XIAO_LORA_SYNC_WORD, XIAO_LORA_TX_POWER_DBM,
                  XIAO_LORA_PREAMBLE_LEN, XIAO_LORA_TCXO_VOLTAGE,
                  XIAO_LORA_USE_REGULATOR_LDO);
  if (lastRadioState != RADIOLIB_ERR_NONE) {
    Serial.printf("failed, code=%d (%s)\n", lastRadioState,
                  radioStateText(lastRadioState));
    if (lastRadioState == RADIOLIB_ERR_CHIP_NOT_FOUND) {
      Serial.println(
          F("No SX1262 SPI response: check the ESP32-S3 B2B module and connector."));
    }
    return false;
  }

  radioReady = true;
  radio.setCurrentLimit(XIAO_LORA_CURRENT_LIMIT_MA);
  radio.setDio2AsRfSwitch(true);
  radio.setPacketReceivedAction(setPacketReceivedFlag);
  Serial.println(F("ok."));
  return startListening();
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(250);

  Serial.println();
  Serial.println(NODE_NAME);
  Serial.println(F("Antenna required before the timed probe test."));
  Serial.println(
      F("Timed wake test: Commands: p=probe, s=status, r=retry radio are unavailable."));
  Serial.printf(
      "Pins NSS=%d DIO1=%d RST=%d BUSY=%d RF_SW=%d SCK=%d MISO=%d MOSI=%d "
      "button=%d\n",
      XIAO_LORA_NSS_PIN, XIAO_LORA_DIO1_PIN, XIAO_LORA_RST_PIN,
      XIAO_LORA_BUSY_PIN, XIAO_LORA_RF_SWITCH_PIN, XIAO_LORA_SPI_SCK_PIN,
      XIAO_LORA_SPI_MISO_PIN, XIAO_LORA_SPI_MOSI_PIN,
      XIAO_LORA_USER_BUTTON_PIN);

  pinMode(XIAO_LORA_USER_BUTTON_PIN, INPUT_PULLUP);
  radioReady = initRadio();
  if (radioReady) {
    sendManualProbe();
  }
  printStatus();
  enterDeepSleep();
}

void loop() {
  serviceSerial();
  serviceButton();
  serviceReceivedPacket();
  serviceStatus();
}
