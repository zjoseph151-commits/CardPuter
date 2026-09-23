#include "app.h"

#include <RadioLib.h>
#include "utility/PI4IOE5V6408_Class.hpp"

namespace {

m5::PI4IOE5V6408_Class messageIo(LORA_IO_EXPANDER_ADDRESS, 400000,
                                  &m5::In_I2C);
SX1262 messageRadio = new Module(LORA_NSS_PIN, LORA_IRQ_PIN, LORA_RST_PIN,
                                  LORA_BUSY_PIN);
volatile bool messageDio1Pending = false;
uint32_t pendingSequence = 0;
unsigned long ackStartedMs = 0;
unsigned long lastManualSendMs = 0;

#if defined(ESP32)
void IRAM_ATTR onMessageDio1() {
#else
void onMessageDio1() {
#endif
  messageDio1Pending = true;
}

bool validSequence(const String& field, uint32_t& sequence) {
  if (field.length() == 0 || field.length() > 10) {
    return false;
  }
  uint64_t value = 0;
  for (int i = 0; i < field.length(); ++i) {
    const char c = field.charAt(i);
    if (c < '0' || c > '9') {
      return false;
    }
    value = value * 10 + (c - '0');
    if (value > UINT32_MAX) {
      return false;
    }
  }
  sequence = static_cast<uint32_t>(value);
  return true;
}

bool validBody(const String& body) {
  if (body.isEmpty() || body.length() > LORA_MESSAGE_TEXT_MAX_CHARS) {
    return false;
  }
  for (int i = 0; i < body.length(); ++i) {
    const char c = body.charAt(i);
    if (c < 32 || c > 126) {
      return false;
    }
  }
  return true;
}

bool splitHeader(const String& packet, const char* prefix, String& sender,
                 String& target, uint32_t& sequence, String* body) {
  if (!packet.startsWith(prefix)) {
    return false;
  }
  int start = strlen(prefix);
  int end = packet.indexOf(',', start);
  if (end < 0) return false;
  sender = packet.substring(start, end);
  start = end + 1;
  end = packet.indexOf(',', start);
  if (end < 0) return false;
  target = packet.substring(start, end);
  start = end + 1;
  end = packet.indexOf(',', start);
  if (body == nullptr) {
    return end < 0 && validSequence(packet.substring(start), sequence);
  }
  if (end < 0 || !validSequence(packet.substring(start, end), sequence)) {
    return false;
  }
  *body = packet.substring(end + 1);
  return validBody(*body);
}

void pushHistory(const String& sender, const String& body, uint32_t sequence,
                 bool outgoing) {
  for (int i = min(static_cast<int>(loraMessageHistoryCount),
                   static_cast<int>(LORA_MESSAGE_HISTORY_SIZE) - 1);
       i > 0; --i) {
    loraMessageHistory[i] = loraMessageHistory[i - 1];
  }
  LoraMessageEntry& entry = loraMessageHistory[0];
  entry.sender = sender;
  entry.body = body;
  entry.sequence = sequence;
  entry.outgoing = outgoing;
  entry.delivered = false;
  entry.receivedMs = millis();
  if (loraMessageHistoryCount < LORA_MESSAGE_HISTORY_SIZE) {
    ++loraMessageHistoryCount;
  }
  loraMessageSelectedIndex = 0;
}

bool startListening() {
  messageDio1Pending = false;
  loraMessageRadioState = messageRadio.startReceive();
  loraMessageListening = loraMessageRadioState == RADIOLIB_ERR_NONE;
  if (!loraMessageListening) {
    loraMessageStatus = String("Listen failed ") + loraMessageRadioState;
  }
  return loraMessageListening;
}

bool transmitFrame(const String& frame) {
  if (!loraMessageRadioReady) return false;
  loraMessageListening = false;
  loraMessageTransmitting = true;
  messageDio1Pending = false;
  String payload = frame;
  loraMessageRadioState = messageRadio.transmit(payload);
  loraMessageTransmitting = false;
  messageDio1Pending = false;
  const bool sent = loraMessageRadioState == RADIOLIB_ERR_NONE;
  if (!sent) {
    loraMessageStatus = String("TX failed ") + loraMessageRadioState;
  }
  startListening();
  return sent;
}

void handleFrame(const String& frame) {
  String sender, target, body;
  uint32_t sequence = 0;
  if (splitHeader(frame, "SCBR,MACK,1,", sender, target, sequence, nullptr)) {
    if (sender != LORA_MESSAGE_PEER_ID ||
        target != LORA_MESSAGE_DEVICE_ID || !loraMessageAwaitingAck ||
        sequence != pendingSequence) {
      return;
    }
    loraMessageAwaitingAck = false;
    ++loraMessageAckCount;
    loraMessageStatus = String("Delivered #") + sequence;
    for (uint8_t i = 0; i < loraMessageHistoryCount; ++i) {
      if (loraMessageHistory[i].outgoing &&
          loraMessageHistory[i].sequence == sequence) {
        loraMessageHistory[i].delivered = true;
        break;
      }
    }
    return;
  }
  if (!splitHeader(frame, "SCBR,MSG,1,", sender, target, sequence, &body) ||
      sender != LORA_MESSAGE_PEER_ID || target != LORA_MESSAGE_DEVICE_ID) {
    return;
  }
  bool duplicate = false;
  for (uint8_t i = 0; i < loraMessageHistoryCount; ++i) {
    if (!loraMessageHistory[i].outgoing &&
        loraMessageHistory[i].sender == sender &&
        loraMessageHistory[i].sequence == sequence) {
      duplicate = true;
      break;
    }
  }
  if (!duplicate) {
    ++loraMessageRxCount;
    pushHistory(sender, body, sequence, false);
    loraMessageStatus = String("Received #") + sequence;
    Serial.printf("LoRa message from %s: %s\n", sender.c_str(), body.c_str());
  }
  const String ack = String("SCBR,MACK,1,") + LORA_MESSAGE_DEVICE_ID + "," +
                     sender + "," + sequence;
  transmitFrame(ack);
}

}  // namespace

void showLoraMessages() {
  if (!loraMessageInitialized) initLoraMessages();
  serviceLoraMessages();
  renderLoraMessages();
}

void renderLoraMessages() {
  lastLoraMessageRenderMs = millis();
  beginContentDraw();
  contentCanvas.setFont(&fonts::Font0);
  contentCanvas.printf("LoRa Messages  TX:%lu ACK:%lu RX:%lu\n",
                       static_cast<unsigned long>(loraMessageTxCount),
                       static_cast<unsigned long>(loraMessageAckCount),
                       static_cast<unsigned long>(loraMessageRxCount));
  contentCanvas.println(loraMessageStatus.substring(0, 36));
  if (loraMessageComposing) {
    contentCanvas.println("To: XIAO  Enter send  Del erase");
    contentCanvas.printf("%u/%u chars\n", loraMessageDraft.length(),
                         LORA_MESSAGE_TEXT_MAX_CHARS);
    for (int i = 0; i < loraMessageDraft.length(); i += 34) {
      contentCanvas.println(loraMessageDraft.substring(i, i + 34));
    }
    contentCanvas.println("Del on empty: cancel");
  } else {
    contentCanvas.println("N/OK compose  Arrows select  R retry");
    if (loraMessageHistoryCount == 0) {
      contentCanvas.println("No messages yet");
    } else {
      const LoraMessageEntry& entry =
          loraMessageHistory[loraMessageSelectedIndex];
      contentCanvas.printf("%u/%u %s #%lu %s\n",
                           loraMessageSelectedIndex + 1,
                           loraMessageHistoryCount,
                           entry.outgoing ? "To XIAO" : "From XIAO",
                           static_cast<unsigned long>(entry.sequence),
                           entry.outgoing ? (entry.delivered ? "ACK" : "pending")
                                          : "RX");
      for (int i = 0; i < entry.body.length(); i += 34) {
        contentCanvas.println(entry.body.substring(i, i + 34));
      }
    }
  }
  commitContentDraw();
  contentCanvas.setFont(&fonts::Font2);
}

bool initLoraMessages() {
  loraMessageInitialized = true;
  loraMessageRadioReady = false;
  loraMessageListening = false;
  loraMessageAwaitingAck = false;
  loraMessageStatus = "Starting radio";
  if (loraMessageSequence == 0) loraMessageSequence = esp_random();
  deselectSharedSpiDevices();
  SPI.end();
  delay(2);
  SPI.begin(LORA_SPI_SCK_PIN, LORA_SPI_MISO_PIN, LORA_SPI_MOSI_PIN,
            LORA_NSS_PIN);
  deselectSharedSpiDevices();
  sharedSpiOwner = SHARED_SPI_OWNER_LORA;
  if (!(m5::In_I2C.isEnabled() || m5::In_I2C.begin()) || !messageIo.begin()) {
    loraMessageStatus = "LoRa I/O not found";
    return false;
  }
  messageIo.setDirection(LORA_RF_SWITCH_PIN, true);
  messageIo.setHighImpedance(LORA_RF_SWITCH_PIN, false);
  messageIo.digitalWrite(LORA_RF_SWITCH_PIN, true);
  loraMessageRadioState = messageRadio.begin(
      LORA_DIAG_RX_FREQUENCY_MHZ, LORA_DIAG_BANDWIDTH_KHZ,
      LORA_DIAG_SPREADING_FACTOR, LORA_DIAG_CODING_RATE, LORA_DIAG_SYNC_WORD,
      LORA_DIAG_UNUSED_TX_POWER_DBM, LORA_DIAG_PREAMBLE_LEN, 3.0, true);
  if (loraMessageRadioState != RADIOLIB_ERR_NONE) {
    loraMessageStatus = String("Radio init failed ") + loraMessageRadioState;
    return false;
  }
  loraMessageRadioReady = true;
  messageRadio.setCurrentLimit(140);
  messageRadio.setDio1Action(onMessageDio1);
  loraMessageStatus = "Listening";
  return startListening();
}

void serviceLoraMessages() {
  if (!loraMessageInitialized) return;
  const unsigned long now = millis();
  if (now - lastLoraMessageServiceMs < LORA_MESSAGE_SERVICE_INTERVAL_MS) return;
  lastLoraMessageServiceMs = now;
  if (messageDio1Pending && loraMessageListening) {
    messageDio1Pending = false;
    loraMessageListening = false;
    String packet;
    loraMessageRadioState = messageRadio.readData(packet);
    if (loraMessageRadioState == RADIOLIB_ERR_NONE) {
      loraMessageLastRssi = messageRadio.getRSSI();
      loraMessageLastSnr = messageRadio.getSNR();
      handleFrame(packet);
    } else if (loraMessageRadioState == RADIOLIB_ERR_CRC_MISMATCH) {
      loraMessageStatus = "CRC mismatch";
    } else {
      loraMessageStatus = String("RX failed ") + loraMessageRadioState;
    }
    if (!loraMessageListening) startListening();
  }
  if (loraMessageAwaitingAck &&
      now - ackStartedMs >= LORA_MESSAGE_ACK_WINDOW_MS) {
    loraMessageAwaitingAck = false;
    ++loraMessageTimeoutCount;
    loraMessageStatus = String("No ACK for #") + pendingSequence;
  }
  if (currentScreen == Screen::LoraMessages &&
      now - lastLoraMessageRenderMs >= LORA_MESSAGE_RENDER_INTERVAL_MS) {
    renderLoraMessages();
  }
}

void stopLoraMessages() {
  if (loraMessageRadioReady) {
    messageRadio.clearDio1Action();
    messageRadio.sleep();
  }
  deselectSharedSpiDevices();
  loraMessageInitialized = false;
  loraMessageRadioReady = false;
  loraMessageListening = false;
  loraMessageTransmitting = false;
  loraMessageAwaitingAck = false;
  loraMessageComposing = false;
  loraMessageDraft = "";
}

void startLoraMessageDraft() {
  loraMessageDraft = "";
  loraMessageComposing = true;
}

void appendLoraMessageCharacter(char c) {
  if (loraMessageComposing && c >= 32 && c <= 126 &&
      loraMessageDraft.length() < LORA_MESSAGE_TEXT_MAX_CHARS) {
    loraMessageDraft += c;
  }
}

void deleteLoraMessageCharacter() {
  if (loraMessageDraft.length() > 0) {
    loraMessageDraft.remove(loraMessageDraft.length() - 1);
  }
}

void cancelLoraMessageDraft() {
  loraMessageDraft = "";
  loraMessageComposing = false;
}

void sendLoraMessageDraft() {
  if (!validBody(loraMessageDraft)) {
    loraMessageStatus = "Type 1-64 ASCII chars";
    return;
  }
  if (!loraMessageRadioReady || loraMessageAwaitingAck) {
    loraMessageStatus = "Radio busy/unavailable";
    return;
  }
  const unsigned long now = millis();
  if (lastManualSendMs && now - lastManualSendMs <
                              LORA_MESSAGE_SEND_COOLDOWN_MS) {
    loraMessageStatus = String("Wait ") +
                        ((LORA_MESSAGE_SEND_COOLDOWN_MS -
                          (now - lastManualSendMs) + 999) / 1000) + "s";
    return;
  }
  const uint32_t sequence = ++loraMessageSequence;
  const String frame = String("SCBR,MSG,1,") + LORA_MESSAGE_DEVICE_ID + "," +
                       LORA_MESSAGE_PEER_ID + "," + sequence + "," +
                       loraMessageDraft;
  loraMessageStatus = String("Transmitting #") + sequence;
  renderLoraMessages();
  if (!transmitFrame(frame)) return;
  ++loraMessageTxCount;
  lastManualSendMs = now;
  pendingSequence = sequence;
  ackStartedMs = millis();
  loraMessageAwaitingAck = true;
  pushHistory(LORA_MESSAGE_DEVICE_ID, loraMessageDraft, sequence, true);
  loraMessageComposing = false;
  loraMessageDraft = "";
  loraMessageStatus = String("Sent #") + sequence + "; waiting ACK";
}

void moveLoraMessageSelection(int direction) {
  if (loraMessageHistoryCount == 0) return;
  const int next = static_cast<int>(loraMessageSelectedIndex) + direction;
  loraMessageSelectedIndex = static_cast<uint8_t>(
      constrain(next, 0, static_cast<int>(loraMessageHistoryCount) - 1));
}
