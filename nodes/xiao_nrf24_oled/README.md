# XIAO ESP32-C3 NRF24 OLED Test Node

This is the archived second NRF24L01 proof node for the Scoober Cardputer firmware.
The active Cardputer feature has moved to `RF Scan`; keep this node for future bench testing.

It uses:

- Seeed Studio XIAO ESP32-C3
- NRF24L01+ PA+LNA module with adapter/breakout
- 0.96 inch SSD1306 128x64 I2C OLED

## Wiring

OLED:

- SDA -> XIAO GPIO6 / SDA
- SCL -> XIAO GPIO7 / SCL
- VCC -> 3V3 or the OLED module's required VCC
- GND -> GND

NRF24L01:

- CE -> XIAO D7 / GPIO20
- CSN -> XIAO D8 / GPIO8
- SCK -> XIAO D9 / GPIO9
- MOSI -> XIAO D10 / GPIO10
- MISO -> XIAO D6 / GPIO21
- GND -> GND
- VCC -> the NRF24 adapter/breakout's required VCC

Use the adapter/breakout power markings. NRF24L01 modules are sensitive to weak or noisy power.

## Protocol

- Cardputer and XIAO both use shared address `SCBR1`.
- Channel: `76`.
- Data rate: `RF24_250KBPS`.
- PA level: `RF24_PA_MIN` for close-range testing with less TX current draw.
- Payload size: 32 bytes.

The XIAO node sends a `XIAO beacon N` packet about four times per second. It also listens for Cardputer packets, waits briefly for the Cardputer to return to listen mode, and sends three `XIAO ack N` replies.

RF24 hardware ACK is disabled for this proof mode. Use the visible `XIAO beacon N` and `XIAO ack N` packets as the application-level proof.

The OLED separates TX attempts from TX OK/fail counts. It also shows the last RX pipe and FIFO state so a failed transmit path is not hidden behind optimistic counters.

## Build

From this folder:

```sh
python -m platformio run
```

From the repo root:

```sh
python -m platformio run -d nodes/xiao_nrf24_oled
```

## Upload

From this folder:

```sh
python -m platformio run --target upload
```

From the repo root:

```sh
python -m platformio run -d nodes/xiao_nrf24_oled --target upload
```

## Serial Monitor

```sh
python -m platformio device monitor -d nodes/xiao_nrf24_oled --baud 115200
```

## Historical Test And Findings

The original expected proof was:

1. Flash the Cardputer firmware.
2. Flash this XIAO node firmware.
3. Open the Cardputer NRF24 feature.
4. Confirm the XIAO OLED shows `Radio: Ready`.
5. Confirm the Cardputer receives `XIAO beacon N`.
6. Press `T` on the Cardputer.
7. Confirm the XIAO RX count increases.
8. Confirm the XIAO `OK` count rises and `F` stays at 0.
9. Confirm Cardputer receives `XIAO ack N`.

Actual result:

- Cardputer pings reached the XIAO node.
- XIAO TX and OK counts increased, and F stayed at 0.
- Cardputer did not decode XIAO beacon or ack packets.
- Future investigation should start with module swaps, cleaner NRF24 power, a capacitor near the radio, or a third known-good node.
