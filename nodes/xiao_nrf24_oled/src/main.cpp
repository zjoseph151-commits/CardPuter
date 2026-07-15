#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RF24.h>
#include <SPI.h>
#include <Wire.h>

constexpr const char* NODE_NAME = "XIAO NRF24 Node";
constexpr int OLED_WIDTH = 128;
constexpr int OLED_HEIGHT = 64;
constexpr int OLED_RESET_PIN = -1;
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;
constexpr int OLED_SDA_PIN = 6;
constexpr int OLED_SCL_PIN = 7;
constexpr uint32_t OLED_I2C_FREQUENCY = 400000U;

constexpr int NRF24_SPI_SCK_PIN = 9;
constexpr int NRF24_SPI_MISO_PIN = 21;
constexpr int NRF24_SPI_MOSI_PIN = 10;
constexpr int NRF24_CE_PIN = 20;
constexpr int NRF24_CSN_PIN = 8;
constexpr uint32_t NRF24_SPI_FREQUENCY = 4000000;
constexpr uint8_t NRF24_CHANNEL = 76;
constexpr uint8_t NRF24_PAYLOAD_SIZE = 32;
constexpr uint8_t NRF24_RX_PIPE = 1;
constexpr rf24_datarate_e NRF24_DATA_RATE = RF24_250KBPS;
constexpr uint32_t RADIO_SERVICE_INTERVAL_MS = 50;
constexpr uint32_t BEACON_INTERVAL_MS = 250;
constexpr uint32_t DISPLAY_REFRESH_INTERVAL_MS = 500;
constexpr uint32_t REPLY_DELAY_MS = 200;
constexpr uint32_t REPLY_REPEAT_DELAY_MS = 75;
constexpr int REPLY_REPEAT_COUNT = 3;

const uint8_t NRF24_SHARED_ADDRESS[5] = {'S', 'C', 'B', 'R', '1'};

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET_PIN);
RF24 radio(NRF24_SPI_FREQUENCY);

bool displayReady = false;
bool radioReady = false;
bool radioListening = false;
String statusText = "Booting...";
String lastRxPacket = "-";
String lastTxPacket = "-";
uint32_t txCount = 0;
uint32_t txSentCount = 0;
uint32_t txFailCount = 0;
uint32_t rxCount = 0;
uint32_t rxFifoFullCount = 0;
uint32_t lastRadioServiceMs = 0;
uint32_t lastBeaconMs = 0;
uint32_t lastDisplayRefreshMs = 0;
uint8_t lastRxPipe = 0xFF;
rf24_fifo_state_e lastRxFifoState = RF24_FIFO_EMPTY;
bool lastTxOk = false;

bool initDisplay();
bool initRadio();
void renderStatus();
void serviceRadio();
void sendRadioPacket(const char* message);
void sendBeacon();
void sendReply();
void resumeRadioListening();
const char* fifoStateText(rf24_fifo_state_e state);

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println(NODE_NAME);

  displayReady = initDisplay();
  renderStatus();

  initRadio();
  renderStatus();
}

void loop() {
  const uint32_t now = millis();

  if (now - lastRadioServiceMs >= RADIO_SERVICE_INTERVAL_MS) {
    serviceRadio();
  }

  if (radioReady && now - lastBeaconMs >= BEACON_INTERVAL_MS) {
    sendBeacon();
  }

  if (now - lastDisplayRefreshMs >= DISPLAY_REFRESH_INTERVAL_MS) {
    renderStatus();
  }

  delay(5);
}

bool initDisplay() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN, OLED_I2C_FREQUENCY);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS, true, false)) {
    Serial.println("OLED: init failed.");
    statusText = "OLED init failed";
    return false;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("XIAO NRF24");
  display.println("Booting...");
  display.display();
  Serial.println("OLED: ready.");
  return true;
}

bool initRadio() {
  radioReady = false;
  radioListening = false;
  statusText = "Radio init...";

  pinMode(NRF24_CSN_PIN, OUTPUT);
  digitalWrite(NRF24_CSN_PIN, HIGH);
  pinMode(NRF24_CE_PIN, OUTPUT);
  digitalWrite(NRF24_CE_PIN, LOW);

  SPI.begin(NRF24_SPI_SCK_PIN, NRF24_SPI_MISO_PIN, NRF24_SPI_MOSI_PIN,
            NRF24_CSN_PIN);
  delay(5);

  if (!radio.begin(&SPI, NRF24_CE_PIN, NRF24_CSN_PIN)) {
    statusText = "Radio begin failed";
    Serial.println("NRF24: radio hardware not responding.");
    return false;
  }

  if (!radio.isChipConnected()) {
    statusText = "No radio on SPI";
    Serial.println("NRF24: chip not detected on SPI.");
    return false;
  }

  radio.setAddressWidth(5);
  radio.setChannel(NRF24_CHANNEL);
  radio.setPayloadSize(NRF24_PAYLOAD_SIZE);
  radio.setDataRate(NRF24_DATA_RATE);
  radio.setCRCLength(RF24_CRC_16);
  radio.disableDynamicPayloads();
  radio.setPALevel(RF24_PA_MIN);
  radio.setAutoAck(false);
  radio.setRetries(0, 0);
  radio.openWritingPipe(NRF24_SHARED_ADDRESS);
  radio.openReadingPipe(NRF24_RX_PIPE, NRF24_SHARED_ADDRESS);
  radio.flush_tx();
  radio.flush_rx();
  radio.clearStatusFlags(RF24_IRQ_ALL);
  resumeRadioListening();

  radioReady = true;
  lastRxPipe = 0xFF;
  lastRxFifoState = RF24_FIFO_EMPTY;
  lastTxOk = false;
  statusText = "Listening";
  Serial.println("NRF24: ready, shared address SCBR1.");
  return true;
}

void renderStatus() {
  lastDisplayRefreshMs = millis();

  if (!displayReady) {
    return;
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("XIAO NRF24");
  display.print(radioReady ? "Ready " : "No radio ");
  display.print("CH");
  display.print(NRF24_CHANNEL);
  display.print(" CE");
  display.print(NRF24_CE_PIN);
  display.print(" CS");
  display.println(NRF24_CSN_PIN);
  display.print("TX:");
  display.print(txCount);
  display.print(" OK:");
  display.print(txSentCount);
  display.print(" F:");
  display.println(txFailCount);
  display.print("RX:");
  display.print(rxCount);
  display.print(" P:");
  if (lastRxPipe <= 5) {
    display.print(lastRxPipe);
  } else {
    display.print("-");
  }
  display.print(" F");
  display.print(fifoStateText(lastRxFifoState));
  display.print("/");
  display.println(rxFifoFullCount);
  display.print("LR:");
  display.println(lastRxPacket.substring(0, 16));
  display.print("LT:");
  display.println(lastTxPacket.substring(0, 16));
  display.println(statusText.substring(0, 20));
  display.display();
}

void serviceRadio() {
  lastRadioServiceMs = millis();

  if (!radioReady) {
    return;
  }

  if (!radioListening) {
    resumeRadioListening();
  }

  lastRxFifoState = radio.isFifo(false);
  if (radio.rxFifoFull()) {
    rxFifoFullCount++;
  }

  bool received = false;
  uint8_t pipe = 0xFF;
  while (radio.available(&pipe)) {
    char incoming[NRF24_PAYLOAD_SIZE + 1] = {};
    radio.read(incoming, NRF24_PAYLOAD_SIZE);
    incoming[NRF24_PAYLOAD_SIZE] = '\0';
    lastRxPipe = pipe;
    lastRxPacket = String(incoming);
    lastRxPacket.trim();
    if (lastRxPacket.length() == 0) {
      lastRxPacket = "(blank)";
    }
    rxCount++;
    received = true;
    Serial.printf("RX %lu: %s\n", rxCount, lastRxPacket.c_str());
  }

  lastRxFifoState = radio.isFifo(false);
  if (received) {
    statusText = "RX from Cardputer";
    renderStatus();
    delay(REPLY_DELAY_MS);
    sendReply();
  }
}

void sendRadioPacket(const char* message) {
  if (!radioReady) {
    return;
  }

  char packet[NRF24_PAYLOAD_SIZE] = {};
  strlcpy(packet, message, sizeof(packet));
  lastTxPacket = String(packet);

  radio.stopListening();
  radioListening = false;
  txCount++;

  radio.clearStatusFlags(RF24_IRQ_ALL);
  lastTxOk = radio.write(packet, sizeof(packet));
  if (lastTxOk) {
    txSentCount++;
    statusText = "TX ok";
  } else {
    txFailCount++;
    radio.flush_tx();
    statusText = "TX failed";
  }

  Serial.printf("TX %lu %s: %s\n", txCount, lastTxOk ? "OK" : "FAIL",
                lastTxPacket.c_str());
  resumeRadioListening();
  renderStatus();
}

void sendBeacon() {
  lastBeaconMs = millis();

  char message[NRF24_PAYLOAD_SIZE] = {};
  snprintf(message, sizeof(message), "XIAO beacon %lu", txCount + 1);
  sendRadioPacket(message);
}

void sendReply() {
  char message[NRF24_PAYLOAD_SIZE] = {};
  snprintf(message, sizeof(message), "XIAO ack %lu", rxCount);
  for (int i = 0; i < REPLY_REPEAT_COUNT; ++i) {
    sendRadioPacket(message);
    if (i + 1 < REPLY_REPEAT_COUNT) {
      delay(REPLY_REPEAT_DELAY_MS);
    }
  }
}

void resumeRadioListening() {
  radio.startListening();
  radio.closeReadingPipe(0);
  radioListening = true;
}

const char* fifoStateText(rf24_fifo_state_e state) {
  switch (state) {
    case RF24_FIFO_OCCUPIED:
      return "O";
    case RF24_FIFO_EMPTY:
      return "E";
    case RF24_FIFO_FULL:
      return "F";
    case RF24_FIFO_INVALID:
    default:
      return "?";
  }
}
