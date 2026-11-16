# Flight Tracker, Priority 13

Date: 2026-10-18

## Scope

- The `Flight Tracker` menu screen uses the Cap GNSS UART parser for a fresh search center. It never starts the SX1262 radio.
- Wi-Fi must already be connected through WiFi Connect, which reads `/config/wifi.txt` from microSD. Flight Tracker neither connects Wi-Fi nor reads or writes the SD card.
- `R` makes one manual adsb.lol HTTPS request for a fixed 25 nautical mile radius. Attempts are spaced at least 60 seconds apart while the device remains powered.
- The HTTPS response needs a known `Content-Length` of at most 64 KiB. Since a busy 25 nm search can contain many aircraft, the response is streamed one aircraft at a time into a 3 KiB object buffer and a 1.5 KiB filtered ArduinoJson document; only the 12 nearest positioned aircraft are retained. Large or unsupported responses show an error instead of consuming unbounded memory.
- The list and detail show callsign (falling back to hex ID), distance and true bearing from the GNSS fix used at fetch time, barometric altitude or ground/unknown, groundspeed, and position age. The detail also shows hex ID, type, and registration when present.
- Positions older than five minutes are excluded. A snapshot or displayed positions older than two minutes get a stale warning; results are never automatically refreshed. No-fix, offline, empty, loading, malformed/oversize response, and HTTP errors are explicit.
- LCD and OLED state that the data is not for navigation. The source is attributed to adsb.lol / ODbL.

## API And Terms Checked

- [adsb.lol OpenAPI](https://api.adsb.lol/api/openapi.json) documents `GET /v2/lat/{lat}/lon/{lon}/dist/{radius}`, a 0-250 nm radius, and the `ac` array fields used here: `flight`, `hex`, `lat`, `lon`, `alt_baro`, `gs`, and `seen_pos`.
- [adsb.lol API documentation](https://api.adsb.lol/docs) was rechecked on 2026-11-15. The `/v2/` endpoint is metered; unkeyed traffic is allowed up to 500 requests/day per IP, and over-quota responses use HTTP 429 with `Retry-After`. The API/data remain ODbL 1.0. This firmware is an experimental, manual-use client; reassess terms before production distribution.
- [adsb.lol privacy and license notice](https://www.adsb.lol/privacy-license/) disclaims accuracy, timeliness, and fitness. This display is not suitable for navigation, collision avoidance, or operational decisions.
- HTTPS currently chains to ISRG Root X1. `include/adsb_lol_ca.h` embeds the [official Let's Encrypt root](https://letsencrypt.org/certificates/); recheck on a certificate-chain change. GNSS UTC sets the TLS clock before the request.

## Controls

- Arrows select an aircraft in the list and move between aircraft in detail.
- OK opens detail. Backspace returns from detail to list, then from list to menu.
- `R` manually searches or refreshes. A missing fresh fix, invalid GNSS UTC, or disconnected Wi-Fi prevents a request.

## Hardware Checklist (Pending)

- With Wi-Fi disconnected and with no GNSS fix, confirm no request is made and the correct state is shown.
- Connect via WiFi Connect and acquire a fresh GNSS fix/UTC; press `R`, check loading and a populated or empty result.
- Check callsign fallback, distance/bearing, altitude/ground/unknown, groundspeed, age, list selection, detail, and Backspace navigation.
- Confirm a second `R` within 60 seconds is refused, then confirm a later manual refresh works.
- Leave a result on screen for over two minutes and confirm the stale warning; then lose GNSS or Wi-Fi and confirm the state changes clearly.
- Test an API error or blocked endpoint, an oversized/unknown-length response if practicable, missing OLED, and low-aircraft area.
- Compare one displayed aircraft position/distance with a contemporaneous independent source for a reasonableness check, not a safety certification.

## 2026-11-15 Parser Fix

- A hardware search reached the API but displayed `API data too complex`. The original parser held filtered fields for the entire `ac` array in a 32 KiB document before retaining 12 aircraft. A representative 25 nm response had 91 aircraft in 44 KiB.
- The response now frames each aircraft object within a 3 KiB bound, validates it with ArduinoJson, retains nearest entries, and discards the object before reading the next. Hardware retest is pending.
- A follow-up hardware search showed `API response incomplete`. The bounded reader now polls available TLS data in blocks, tolerates temporary zero-reads for up to 20 seconds of inactivity, and shows received/declared byte counts on the LCD (also logs them to Serial) if the body truly stops early. Retest on device is pending.

Build and static guards pass before hardware validation; no on-device behavior is claimed yet.
