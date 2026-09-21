# LoRa TX Planning

Date: 2026-09-13

This began as the planning gate for transmit-capable Priority #11 work. `LoRa Range Test is the only Cardputer TX feature` now implemented from that gate; it is a manual, armed ping/ACK test rather than a general transmit facility. The separate XIAO companion node is its second ACK/probe node during bench testing.

## Sources Checked

- eCFR 47 CFR 15.5, general Part 15 operating conditions: https://www.ecfr.gov/current/title-47/chapter-I/subchapter-A/part-15/subpart-A/section-15.5
- eCFR 47 CFR 15.23, home-built devices: https://www.ecfr.gov/current/title-47/chapter-I/subchapter-A/part-15
- eCFR 47 CFR 15.247, 902-928 MHz frequency hopping and digitally modulated intentional radiators: https://www.ecfr.gov/current/title-47/chapter-I/subchapter-A/part-15/subpart-C/subject-group-ECFR2f2e5828339709e/section-15.247
- eCFR 47 CFR 15.249, 902-928 MHz field-strength option: https://www.ecfr.gov/current/title-47/chapter-I/subchapter-A/part-15/subpart-C/subject-group-ECFR2f2e5828339709e/section-15.249
- M5Stack Cap LoRa-1262 product page: https://shop.m5stack.com/products/cap-lora-1262-for-cardputer-adv-sx1262-atgm336h
- M5Stack Cap LoRa868 / LoRa-1262 Arduino tutorial: https://docs.m5stack.com/en/arduino/projects/cap/cap_lora868
- Seeed Studio Wio-SX1262 for XIAO product page: https://www.seeedstudio.com/Wio-SX1262-for-XIAO-p-6379.html
- Seeed Studio Wio-SX1262 introduction: https://wiki.seeedstudio.com/wio_sx1262/
- Seeed Studio XIAO ESP32S3 & Wio-SX1262 kit introduction: https://wiki.seeedstudio.com/wio_sx1262_with_xiao_esp32s3_kit/
- Seeed Studio standalone Wio-SX1262 for XIAO schematic: https://files.seeedstudio.com/products/SenseCAP/Wio_SX1262/Wio-SX1262%20for%20XIAO%20V1.0_SCH.pdf
- Seeed Studio XIAO ESP32S3 + Wio-SX1262 B2B kit pinout source: https://github.com/Seeed-Studio/one_channel_hub/blob/4cc771ac02da1bd18be67509f6b52d21bb0feabd/components/smtc_ral/bsp/sx126x/seeed_xiao_esp32s3_devkit_sx1262.c

## Regulatory Planning Notes

- Treat this as an engineering planning note, not a legal determination.
- The working assumption is United States use under the US 902-928 MHz band because the user is in the United States and the current RX diagnostics use 915.0 MHz.
- Under 47 CFR 15.5, the device must not cause harmful interference, must accept interference, and must stop operating if directed because it is causing harmful interference.
- 47 CFR 15.23 gives a limited home-built-device path for personal, non-marketed devices in quantities of five or fewer, but it still expects good engineering practice and points back to the 47 CFR 15.5 operating conditions.
- 47 CFR 15.247 and 47 CFR 15.249 are the official rule areas to review before any public or continuous TX use. The firmware should stay conservative because simple single-channel 125 kHz LoRa settings do not automatically prove compliance with every 15.247 or 15.249 measurement requirement.
- If the device travels outside the United States, stop TX work and choose the local legal region, frequency plan, bandwidth/spreading plan, and power limit before transmitting.

## Hardware Plan

- Antenna must be installed before any TX test.
- Use the included Cap LoRa-1262 SMA antenna for the first tests. M5Stack lists that antenna as 3 dBi, which keeps the initial plan tied to known hardware.
- Do not change antennas without revisiting gain, conducted power, and radiated power assumptions.
- Use the XIAO ESP32-S3 Wio-SX1262 LoRa ACK Node as the first second-node bench partner. The standalone Wio-SX1262 for XIAO uses the normal XIAO headers, while the separate ESP32S3 kit uses B2B; select the matching PlatformIO environment before hardware bring-up.
- Keep the XIAO companion node at the same conservative 2 dBm initial TX power even though the Wio-SX1262 hardware is capable of higher output power.
- Keep the current EXT/SPI ownership model: LoRa owns `G40/G39/G14` only while a LoRa screen is active, microSD owns the bus during SD-backed screens, and each feature must hand the bus back cleanly.
- Keep `G8/G9` reserved for internal Cardputer/cap I2C and the PI4IOE5V6408 path. The OLED/ENV/RTC PaHub path remains on the Grove side.

## Initial Radio Settings

Start the first TX-capable feature by matching the known-good RX monitor settings:

- Region: US 902-928 MHz.
- Frequency: 915.0 MHz.
- Bandwidth: 125 kHz.
- Spreading factor: SF12.
- Coding rate: CR 4/5.
- Sync word: 0x34, matching the current `LoRa Diag` and `LoRa Packets` settings.
- Preamble length: 20 symbols.
- Initial TX power: 2 dBm, matching the current `LORA_DIAG_UNUSED_TX_POWER_DBM` placeholder used during RX init.

Only raise TX power after a short-range bench test passes, antenna assumptions are still valid, and the selected regional rule path has been reviewed. Never exceed the module, antenna, or legal-region plan.

## Firmware Guardrails

- No background TX in the first implementation.
- No transmit from `LoRa Diag` or `LoRa Packets`; those screens remain RX-only.
- TX must be available only inside the dedicated first transmit feature.
- Show the active frequency, bandwidth, SF, sync word, and TX power before arming TX.
- Require an explicit on-screen arm/confirm step after entering the TX feature because the firmware cannot detect whether the antenna is attached.
- Rate limit manual packets. Start with one user-triggered packet no more often than every 10 seconds.
- Use only canned ASCII payloads until the range-test path is stable.
- After each RadioLib `startTransmit(...)`, call the matching completion path and return the radio to receive or sleep intentionally.
- Clear ISR flags and put the radio to sleep when leaving the screen.
- Keep existing no-TX guards active until the TX feature is deliberately added; then update them to allow TX only in the new TX module while still forbidding TX in RX-only screens.

## XIAO Companion Node

The first bench partner is [nodes/xiao_sx1262_lora_ack](../../../nodes/xiao_sx1262_lora_ack), the XIAO ESP32-S3 Wio-SX1262 LoRa ACK Node.

Scope:

- Separate PlatformIO project using `seeed_xiao_esp32s3` and RadioLib.
- Does not change the active Cardputer firmware or any Cardputer menu item.
- Defaults to the standalone Wio-SX1262 for XIAO header pinout: `GPIO5` NSS, `GPIO2` DIO1/IRQ, `GPIO3` RST, `GPIO4` BUSY, `GPIO1` RF switch, and XIAO SPI `GPIO7/GPIO8/GPIO9`. The optional `xiao_esp32s3_b2b` environment uses the kit's `GPIO41/GPIO39/GPIO42/GPIO40/GPIO38` mapping.
- Uses a 3.0 V TCXO setting and `radio.setDio2AsRfSwitch(true)`, matching Seeed's XIAO ESP32S3 SX1262 pinout/source assumptions.
- Keeps the external RF switch in receive mode while listening and switches it only during transmit.
- Starts in receive mode and has no periodic beacons.
- Sends a manual probe only from the Wio user button or serial `p`: `SCBR,NODE,1,xiao-sx1262-ack,<seq>,<uptime_ms>`.
- Replies to matching Cardputer pings with `SCBR,ACK,1,xiao-sx1262-ack,<seq>,<remote_rssi>,<remote_snr>,<uptime_ms>`.
- Uses the same 915.0 MHz / 125 kHz / SF12 / CR 4/5 / sync word 0x34 / preamble 20 / 2 dBm settings as the Cardputer RX screens.

First hardware check:

- Flash the XIAO companion.
- Confirm serial prints `Initializing SX1262... ok.`
- Open Cardputer `LoRa Packets`.
- Press the Wio user button or type `p` in the XIAO serial monitor.
- Confirm Cardputer packet count, RSSI, SNR, length, age, and payload update from the `SCBR,NODE,1,xiao-sx1262-ack` payload.

## First TX Feature

`LoRa Ping / Range Test` is implemented before pager, beacon, bridge, or map features. It is the only current Cardputer transmit path.

The first pass should:

- Uses the XIAO ESP32-S3 Wio-SX1262 LoRa ACK Node as the second node with matching radio settings.
- `A` creates a new `/tracks/lora-rangeNNN.csv` file, writes its header, and arms the screen. The screen cannot send until the CSV exists.
- `P` sends one canned ASCII payload, `SCBR,PING,1,scoober-cardputer,<seq>,<uptime_ms>,<battery_pct>`, with a 10-second cooldown.
- The radio then listens for `SCBR,ACK,1,xiao-sx1262-ack,<seq>,<remote_rssi>,<remote_snr>,<remote_uptime_ms>` for 10 seconds.
- Shows TX, failure, ACK, timeout, last ACK RSSI/SNR/age, known radio settings, and CSV status on the LCD; the OLED shows the compact command sheet.
- Logs ACKs, ACK timeouts, and TX errors to microSD, using GNSS UTC/date when available, RTC time as a fallback, and uptime in every row.
- `C` clears on-screen counters, OK/Enter or `R` reinitializes the test, and Backspace sleeps the radio and returns to the menu.

The first implementation should not include free-form text entry, periodic beacons, MQTT rebroadcasting, or automatic retries beyond a small explicit retry count.

## Later TX Features

- Scoober Pager: canned messages first, then optional typed messages after range-test stability.
- Location Beacon: disabled by default, explicit enable only, slow interval, optional GNSS position only after the user confirms the privacy/airtime tradeoff.
- MQTT LoRa Bridge: RX-to-MQTT first; any Wi-Fi-to-LoRa TX must whitelist topics/actions and prevent rebroadcast loops.
- Signal Map: consume a known beacon or range-test ACKs and log GNSS position plus RSSI/SNR.

## Acceptance Constraints For TX Firmware

- Antenna is physically installed and the user confirms it before testing.
- Region is set to US 902-928 MHz for the current device location.
- Initial TX settings match the RX monitors: 915.0 MHz, 125 kHz, SF12, CR 4/5, sync word 0x34, preamble 20.
- Initial TX power is 2 dBm and any increase is treated as a separate deliberate choice.
- First feature is `LoRa Ping / Range Test`.
- Packet payloads are canned ASCII.
- Manual TX has a visible arm state and rate limit.
- The XIAO ESP32-S3 Wio-SX1262 LoRa ACK Node is hardware-validated for manual `P` probes received by Cardputer `LoRa Packets`.
- Cardputer `LoRa Ping / Range Test` still needs its dedicated hardware ACK/CSV test before any later TX-capable feature is considered.
- Existing RX-only features still have no transmit action.
