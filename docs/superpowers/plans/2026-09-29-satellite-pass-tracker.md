# Satellite Pass Tracker

Date: 2026-09-29

## Baseline

- The user confirmed the LoRa diagnostic and messaging changes are working on hardware. Priority #11 can proceed to satellite passes without changing those radio screens.
- Reuse the Cap LoRa-1262 GNSS parser for observer latitude, longitude, altitude, and UTC. The tracker must not initialize or transmit on SX1262.
- Use the existing Wi-Fi Connect flow; the pass tracker never reads credentials or connects Wi-Fi itself.

## First Pass

- Select ISS (25544), NOAA 18 (28654), NOAA 19 (33591), or a user-entered catalog number supported by TLE (under 70000). Persist the custom number in Preferences.
- Retrieve the selected object only on `U`, using CelesTrak `gp.php?CATNR=<id>&FORMAT=TLE`, and never more than once per object per two hours, including failed HTTP responses and restarts. A missing Wi-Fi connection does not consume an attempt. CelesTrak's default format is now CSV, so `FORMAT=TLE` is mandatory. CelesTrak says it checks for new GP data every two hours and may block more frequent requests.
- Validate the returned two-line set (line shape, catalog number, checksums), then replace `/config/sat<id>.txt` only after a complete download. Load the saved copy offline. Reject elements older than 14 days for predictions and label data older than three days.
- Use the Hopperpop Arduino SGP4 implementation for orbit propagation and its next-pass search. Show next rise, peak, set, maximum elevation, and azimuth in UTC for passes reaching at least 10 degrees. This is an approximate pointing aid, not an observation guarantee.
- Keep the source of observer location and time visible. Require a fresh GNSS fix and valid UTC for calculations; do not silently substitute an old location or build time.
- OLED shows concise commands and selected satellite/pass state. No network polling in the background.

## Verification

- Build Cardputer, run the guard suite, and check `git diff --check`.
- On hardware: test empty SD, offline cache, Wi-Fi absent, no GNSS fix, invalid UTC, a successful TLE download, rate limit, all built-in targets, a custom target, and SD Manager inspection of a cache file.
- Compare an ISS pass time/azimuth against an independent current SGP4 tool at the same coordinates and TLE epoch before relying on the forecast.

## Sources

- CelesTrak GP query and format documentation: https://celestrak.org/NORAD/documentation/gp-data-formats.php
- CelesTrak current data and TLE limitation: https://celestrak.org/NORAD/elements/
- SGP4 Arduino library: https://github.com/Hopperpop/Sgp4-Library
