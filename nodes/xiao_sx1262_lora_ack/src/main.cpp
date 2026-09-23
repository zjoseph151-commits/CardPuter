#include <Arduino.h>
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
constexpr uint32_t XIAO_LORA_MESSAGE_ACK_WINDOW_MS = 12000;
constexpr uint32_t XIAO_LORA_STATUS_INTERVAL_MS = 5000;
constexpr uint32_t XIAO_LORA_BUTTON_DEBOUNCE_MS = 40;
constexpr const char* CARDPUTER_ID = "scoober-cardputer";
constexpr uint8_t MESSAGE_MAX_CHARS = 64;

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
uint32_t lastStatusMs = 0;
RTC_DATA_ATTR uint32_t manualSequence = 0;
uint32_t messageSequence = 0;
uint32_t pendingMessageSequence = 0;
uint32_t messageAckStartedMs = 0;
bool awaitingMessageAck = false;
String serialCommand;
uint32_t recentMessageSequences[6] = {};
uint8_t recentMessageCount = 0;
uint32_t rxCount = 0;
uint32_t crcErrorCount = 0;
uint32_t rxErrorCount = 0;
uint32_t txCount = 0;
uint32_t txFailCount = 0;
uint32_t ackCount = 0;
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
    Serial.printf("Manual TX rate-limited; wait %lu s.\n",
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

bool parseMessage(const String& payload, String& sender, String& target,
                  uint32_t& sequence, String& body) {
  if (!payload.startsWith("SCBR,MSG,1,")) return false;
  int start = 11;
  int end = payload.indexOf(',', start);
  if (end < 0) return false;
  sender = payload.substring(start, end);
  start = end + 1;
  end = payload.indexOf(',', start);
  if (end < 0) return false;
  target = payload.substring(start, end);
  start = end + 1;
  end = payload.indexOf(',', start);
  if (end < 0) return false;
  const String sequenceText = payload.substring(start, end);
  if (sequenceText.isEmpty() || sequenceText.length() > 10) return false;
  uint64_t parsed = 0;
  for (int i = 0; i < sequenceText.length(); ++i) {
    const char c = sequenceText.charAt(i);
    if (c < '0' || c > '9') return false;
    parsed = parsed * 10 + c - '0';
    if (parsed > UINT32_MAX) return false;
  }
  sequence = static_cast<uint32_t>(parsed);
  body = payload.substring(end + 1);
  if (body.isEmpty() || body.length() > MESSAGE_MAX_CHARS) return false;
  for (int i = 0; i < body.length(); ++i) {
    if (body.charAt(i) < 32 || body.charAt(i) > 126) return false;
  }
  return true;
}

void sendMessage(const String& body) {
  if (body.isEmpty() || body.length() > MESSAGE_MAX_CHARS) {
    Serial.println(F("Message must be 1-64 printable ASCII characters."));
    return;
  }
  for (int i = 0; i < body.length(); ++i) {
    if (body.charAt(i) < 32 || body.charAt(i) > 126) {
      Serial.println(F("Only printable ASCII is supported."));
      return;
    }
  }
  if (awaitingMessageAck) {
    Serial.println(F("Waiting for the previous message ACK."));
    return;
  }
  const uint32_t sequence = messageSequence + 1;
  const String payload = String("SCBR,MSG,1,") + DEVICE_ID + "," +
                         CARDPUTER_ID + "," + sequence + "," + body;
  if (sendPayload(payload, true)) {
    messageSequence = sequence;
    pendingMessageSequence = sequence;
    awaitingMessageAck = true;
    messageAckStartedMs = millis();
  }
}

void handlePacket(const String& payload) {
  String sender, target, body;
  uint32_t sequence = 0;
  if (parseMessage(payload, sender, target, sequence, body) &&
      sender == CARDPUTER_ID && target == DEVICE_ID) {
    bool duplicate = false;
    for (uint8_t i = 0; i < recentMessageCount; ++i) {
      if (recentMessageSequences[i] == sequence) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate) {
      Serial.printf("Message from Cardputer #%lu: %s\n",
                    static_cast<unsigned long>(sequence), body.c_str());
      for (int i = min(static_cast<int>(recentMessageCount), 5); i > 0; --i) {
        recentMessageSequences[i] = recentMessageSequences[i - 1];
      }
      recentMessageSequences[0] = sequence;
      if (recentMessageCount < 6) ++recentMessageCount;
    }
    const String ack = String("SCBR,MACK,1,") + DEVICE_ID + "," + sender +
                       "," + sequence;
    if (sendPayload(ack, false)) ++ackCount;
  } else if (payload.startsWith("SCBR,MACK,1,")) {
    const String expectedAck = String("SCBR,MACK,1,") + CARDPUTER_ID + "," +
                               DEVICE_ID + "," + pendingMessageSequence;
    if (awaitingMessageAck && payload == expectedAck) {
      awaitingMessageAck = false;
      Serial.printf("Message #%lu delivered.\n",
                    static_cast<unsigned long>(pendingMessageSequence));
    }
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
      "fail=%lu ack=%lu wait=%s state_text=%s freq=%.1f bw=%.0f sf=%u cr=4/%u sw=0x%02X "
      "pwr=%d\n",
      radioReady ? "ready" : "down", listening ? "yes" : "no", lastRadioState,
      static_cast<unsigned long>(rxCount),
      static_cast<unsigned long>(crcErrorCount),
      static_cast<unsigned long>(rxErrorCount),
      static_cast<unsigned long>(txCount),
      static_cast<unsigned long>(txFailCount),
      static_cast<unsigned long>(ackCount),
      awaitingMessageAck ? "yes" : "no",
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

void runSerialCommand(const String& command) {
  if (command == "p" || command == "P") {
    sendManualProbe();
  } else if (command == "s" || command == "S") {
    printStatus();
  } else if (command == "r" || command == "R") {
    Serial.println(F("Retrying SX1262 initialization..."));
    initRadio();
    printStatus();
  } else if (command.startsWith("m ") || command.startsWith("M ")) {
    sendMessage(command.substring(2));
  } else if (!command.isEmpty()) {
    Serial.println(F("Commands: m <text>, p, s, r"));
  }
}

void serviceSerial() {
  while (Serial.available() > 0) {
    const char key = static_cast<char>(Serial.read());
    if (key == '\r' || key == '\n') {
      if (!serialCommand.isEmpty()) runSerialCommand(serialCommand);
      serialCommand = "";
    } else if (key >= 32 && key <= 126 && serialCommand.length() < 80) {
      serialCommand += key;
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
  if (awaitingMessageAck &&
      now - messageAckStartedMs >= XIAO_LORA_MESSAGE_ACK_WINDOW_MS) {
    awaitingMessageAck = false;
    Serial.printf("No ACK for message #%lu.\n",
                  static_cast<unsigned long>(pendingMessageSequence));
  }
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
  Serial.println(F("Antenna required. Awake messaging test peer."));
  Serial.println(F("Commands: m <text>, p=probe, s=status, r=retry radio."));
  Serial.printf(
      "Pins NSS=%d DIO1=%d RST=%d BUSY=%d RF_SW=%d SCK=%d MISO=%d MOSI=%d "
      "button=%d\n",
      XIAO_LORA_NSS_PIN, XIAO_LORA_DIO1_PIN, XIAO_LORA_RST_PIN,
      XIAO_LORA_BUSY_PIN, XIAO_LORA_RF_SWITCH_PIN, XIAO_LORA_SPI_SCK_PIN,
      XIAO_LORA_SPI_MISO_PIN, XIAO_LORA_SPI_MOSI_PIN,
      XIAO_LORA_USER_BUTTON_PIN);

  pinMode(XIAO_LORA_USER_BUTTON_PIN, INPUT_PULLUP);
  messageSequence = esp_random();
  radioReady = initRadio();
  printStatus();
}

void loop() {
  serviceSerial();
  serviceButton();
  serviceReceivedPacket();
  serviceStatus();
}
