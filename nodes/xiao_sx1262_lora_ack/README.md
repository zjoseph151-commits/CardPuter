# XIAO ESP32-S3 Wio-SX1262 LoRa ACK Node

This is the planned companion node for Scoober's first transmit-capable
Priority #11 feature, `LoRa Ping / Range Test`.

It is intentionally separate from the active Cardputer firmware. The Cardputer
firmware remains RX-only until the dedicated range-test screen is added.

## Hardware

- Seeed Studio XIAO ESP32-S3
- Seeed Studio Wio-SX1262 for XIAO
- 915 MHz LoRa antenna attached before any transmit test
- USB-C data cable for serial monitor and upload

The default pinout targets the Seeed XIAO ESP32S3 + Wio-SX1262 B2B kit:

- NSS: GPIO41
- DIO1 / IRQ: GPIO39
- RST: GPIO42
- BUSY: GPIO40
- RF_SW: GPIO38
- SCK: XIAO default SPI SCK / GPIO7
- MISO: XIAO default SPI MISO / GPIO8
- MOSI: XIAO default SPI MOSI / GPIO9
- User button: GPIO21

If the hardware revision arrives with a through-header mapping instead of the
B2B mapping, override the `XIAO_LORA_*_PIN` build flags in `platformio.ini`
before uploading.

## Radio Settings

- Frequency: 915.0 MHz
- Bandwidth: 125 kHz
- Spreading factor: SF12
- Coding rate: CR 4/5
- Sync word: 0x34
- Preamble: 20 symbols
- TX power: 2 dBm
- Manual transmit cooldown: 10 seconds

These match the current Cardputer `LoRa Diag` and `LoRa Packets` receive
settings.

## Behavior

- Starts in receive mode and does not send periodic beacons.
- Press the Wio user button, or type `p` in serial, to send one manual probe:
  `SCBR,NODE,1,xiao-sx1262-ack,<seq>,<uptime_ms>`.
- Type `s` in serial to print the current status.
- When it receives a matching Cardputer ping beginning with `SCBR,PING,1,`, it
  replies with `SCBR,ACK,1,xiao-sx1262-ack,<seq>,<rssi>,<snr>,<uptime_ms>`.
- It keeps the RF switch in receive mode while listening and transmit mode only
  during a send.

## Build

From this folder:

```sh
python -m platformio run
```

From the repo root:

```sh
python -m platformio run -d nodes/xiao_sx1262_lora_ack
```

## Upload

Attach the LoRa antenna before using the manual probe or ACK behavior.

From this folder:

```sh
python -m platformio run --target upload
```

From the repo root:

```sh
python -m platformio run -d nodes/xiao_sx1262_lora_ack --target upload
```

## Serial Monitor

```sh
python -m platformio device monitor -d nodes/xiao_sx1262_lora_ack --baud 115200
```

First hardware checks:

1. Confirm serial prints `Initializing SX1262... ok.`
2. Open Cardputer `LoRa Packets`.
3. Press the XIAO/Wio user button or type `p`.
4. Confirm Cardputer packet count, RSSI, SNR, length, age, and payload update.
5. Confirm no periodic packets appear unless a manual probe is sent or a ping is
   received.
