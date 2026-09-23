# XIAO ESP32-S3 Wio-SX1262 LoRa ACK Node

This is the awake serial test peer for Scoober's `LoRa Messages` feature.
It remains a separate PlatformIO project. The former ping/range and 60-second
deep-sleep beacon experiments passed on hardware and are now historical; a
sleeping node could not receive interactive messages.

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
- TX power: 5 dBm, DC-DC regulation
- Manual transmit cooldown: 10 seconds

The Cardputer uses the same modulation settings and 2 dBm TX power.

## Behavior

- Stays awake in receive mode and does not send periodic beacons.
- Press the Wio user button, or type `p` in serial, to send one manual probe:
  `SCBR,NODE,1,xiao-sx1262-ack,<seq>,<uptime_ms>`.
- Type `m <text>` followed by Enter to send a 1-64 character printable ASCII
  message to the Cardputer. The node waits for a matching `SCBR,MACK,1` reply.
- Incoming Cardputer `SCBR,MSG,1` text prints to serial. The node sends a
  matching `SCBR,MACK,1` and ACKs duplicates again without printing them twice.
- Type `s` in serial to print the current status.
- Type `r` in serial to retry SX1262 initialization after checking the board
  connection; this does not require reflashing the XIAO.

Wire format: `SCBR,MSG,1,<sender>,<target>,<sequence>,<body>` and
`SCBR,MACK,1,<sender>,<target>,<sequence>`. Commas in the body are retained.
This is a local test protocol, not LoRaWAN or an encrypted messenger.

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

Attach the LoRa antenna before transmitting messages, probes, or ACKs.

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
2. Open Cardputer `LoRa Diag` and check its packet page.
3. Press the XIAO/Wio user button or type `p`.
4. Confirm Cardputer packet count, RSSI, SNR, length, age, and payload update.
5. Open Cardputer `LoRa Messages`, compose and send text, and confirm it prints
   on node serial and becomes ACKed on Cardputer.
6. Enter `m hello from XIAO` on serial and confirm the Cardputer receives it.
7. Confirm no periodic packets appear while the node is awake and idle.
