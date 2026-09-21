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

The default `xiao_esp32s3` build targets the standalone Wio-SX1262 for XIAO
connected through the normal XIAO headers:

- NSS: GPIO5
- DIO1 / IRQ: GPIO2
- RST: GPIO3
- BUSY: GPIO4
- RF_SW: GPIO1
- SCK: XIAO default SPI SCK / GPIO7
- MISO: XIAO default SPI MISO / GPIO8
- MOSI: XIAO default SPI MOSI / GPIO9
- User button: GPIO21

The separate XIAO ESP32S3 + Wio-SX1262 kit uses the rear B2B connector rather
than the normal headers. Build that version with `-e xiao_esp32s3_b2b`; it uses
NSS 41, DIO1 39, RST 42, BUSY 40, and RF_SW 38.

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
- Type `r` in serial to retry SX1262 initialization after checking the board
  connection; this does not require reflashing the XIAO.
- When it receives a matching Cardputer ping beginning with `SCBR,PING,1,`, it
  replies with `SCBR,ACK,1,xiao-sx1262-ack,<seq>,<rssi>,<snr>,<uptime_ms>`.

## Startup Check

On boot, the serial monitor must include `Initializing SX1262... ok.` and the
status must show `radio=ready`, `listen=yes`, and `state=0`.

`state=-2` / `state_text=chip-not-found` is RadioLib's SPI probe failure: the
ESP32-S3 cannot communicate with the SX1262. It happens before receive or
transmit configuration, so changing frequency, sync word, or pressing `p`
cannot resolve it.

For `chip-not-found`:

- For the standalone Wio-SX1262 product, upload the default header build. It
  uses NSS 5, DIO1 2, RST 3, BUSY 4, RF_SW 1, and SPI GPIO7/GPIO8/GPIO9.
- Use `-e xiao_esp32s3_b2b` only for the separate ESP32-S3 B2B kit. Its
  `GPIO41/GPIO39/GPIO42/GPIO40/GPIO38` mapping cannot detect a header-connected
  module.
- Reconnect USB and type `r` in the serial monitor. A successful retry prints
  `Initializing SX1262... ok.` and changes the status to `radio=ready`.
- If the matching build still cannot initialize, inspect the header soldering
  and module contacts or test with a known-good matching board.
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

For the separate B2B kit, add `-e xiao_esp32s3_b2b` before `--target upload`.

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
