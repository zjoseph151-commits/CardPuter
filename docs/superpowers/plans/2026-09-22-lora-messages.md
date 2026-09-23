# LoRa Diagnostics And Messaging

Date: 2026-09-22

## Verified Baseline

- The Cardputer Cap LoRa-1262 and standalone XIAO ESP32-S3 + Wio-SX1262 have exchanged packets in both directions. The user confirmed the range test works.
- The XIAO's minute probe and deep-sleep test worked. Its node TX setting is now 5 dBm with DC-DC regulation; the Cardputer TX setting remains 2 dBm.
- The separate `LoRa Packets`, `LoRa Diag`, and `LoRa Range` screens are the current implementation, not the intended final menu.

## Screen Changes

1. Combine `LoRa Packets` and `LoRa Diag` into one RX-only `LoRa Diag` feature with Packet and Hardware/GNSS pages. Preserve packet count, CRC/error count, live channel RSSI, packet RSSI/SNR, length, age, sanitized payload, cap expander/RF-switch/radio state, radio settings, GNSS fix/satellites/HDOP/coordinates/time, and NMEA counters. One SX1262 owner runs while this screen is open. `C` clears packet counters, `R` or OK restarts, and Backspace sleeps the radio.
2. Replace `LoRa Range` with `LoRa Messages`. Keep the proven manual transmit/receive lifecycle and matching ACK concept. Remove range CSV creation, arm state, ping UI, and range counters. Existing `/tracks/lora-rangeNNN.csv` files remain untouched.
3. Cardputer messaging uses a short printable ASCII composer, recent in-memory sent/received history, explicit send, matched delivery ACK, timeout/error state, and a manual send cooldown. It receives only while the message screen is open. The OLED shows concise commands and delivery status.
4. Use versioned `SCBR,MSG,1,<sender>,<target>,<sequence>,<body>` and `SCBR,MACK,1,<sender>,<target>,<sequence>` frames. Parse only the fixed header fields so commas in message bodies remain intact. Validate target, length, and characters; ignore duplicate messages in history while ACKing them again.
5. Reconfigure the XIAO as an awake serial test peer. It prints Cardputer messages, sends matching ACKs, and accepts a serial command for a reply. The node screen is future work in another chat. Its minute deep-sleep beacon mode must be disabled for messaging tests because a sleeping node cannot receive.

## Verification

- Firmware status (2026-09-22): the two-page RX-only diagnostic, LoRa Messages composer/history/ACK flow, and awake XIAO serial peer are implemented. The former packet-monitor and range-test source files and menu entries are retired; old SD range CSVs are untouched.
- Cardputer and both XIAO pin profiles build successfully. The current static guard suite passes, including `tools/check_lora_messages.py`. This does not substitute for a hardware exchange test.
- On hardware, send a Cardputer message, confirm its text on XIAO serial, confirm Cardputer delivery ACK, send a XIAO serial reply, and confirm it appears on Cardputer.
- Recheck both pages of merged `LoRa Diag` with matching frames and GNSS data, including graceful missing-radio/no-fix states, OLED help, and shared SPI recovery.
