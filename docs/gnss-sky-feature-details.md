# GNSS Sky Feature Details

Reference notes for the Scoober Cardputer GNSS Sky feature and possible next improvements.

## Current Concept

`GNSS Sky` is a visual satellite-in-view screen for the M5Stack Cap LoRa-1262 GNSS receiver. It uses GNSS NMEA data from the ATGM336H receiver and does not initialize the SX1262 LoRa radio or transmit.

The feature reads `$GxGSV` NMEA sentences, which report satellites currently visible to the GNSS receiver. Each plotted satellite can include:

- Constellation / talker, such as GPS, GLONASS, Galileo, BeiDou, or mixed GNSS
- PRN / satellite ID
- Elevation angle
- Azimuth angle
- SNR signal strength

The sky plot represents the sky above the user:

- The outside circle is the horizon.
- The center is directly overhead.
- Inner rings represent higher elevation.
- North, east, south, and west labels indicate direction.
- Satellite dots are placed using azimuth and elevation.
- Dot color represents SNR strength.

## Important Terms

SNR means signal-to-noise ratio. It describes how clearly the GNSS receiver can hear a satellite compared to background noise. Higher SNR usually means a cleaner, stronger signal. If SNR is missing, the satellite can still be plotted from elevation and azimuth, but signal strength is unknown.

Fix means the GNSS receiver has enough usable satellite timing data to estimate position. Satellites can appear in the sky view before a location fix is available. A 2D fix can estimate latitude and longitude; a 3D fix can also estimate altitude.

`Sky:x/y` means:

- `x`: satellites Scoober currently has enough fresh data to plot.
- `y`: satellites-in-view reported by the GNSS receiver in GSV data.

Latitude and longitude show the Cardputer's estimated current location once the GNSS fix is fresh. Altitude is also from GNSS and is usually noisier than latitude/longitude, especially indoors or near windows.

`Best` currently means the active plotted satellite with the highest reported SNR. It is the strongest signal being heard, not necessarily the satellite most important to the current location fix.

## Proposed Selected-Satellite Improvement

The next improvement should let the user inspect all plotted satellites.

Desired behavior:

- Use the arrow keys to cycle through plotted satellites.
- Highlight the selected satellite dot on the built-in LCD sky plot.
- Keep the right-side built-in LCD panel focused on overall sky/fix summary.
- Use the extra SSD1309 OLED for selected-satellite details instead of putting all selected details in the right-side panel.

Useful selected-satellite details:

- Satellite label, such as `G12`
- Constellation name
- PRN / satellite ID
- SNR
- Elevation in degrees
- Azimuth in degrees
- Compass direction derived from azimuth
- Last-seen age

Potential OLED selected-satellite layout:

```text
GNSS Sky
Sel: G12 GPS
SNR: 38 dB
El: 64 Az:135
SE age:2s
```

## Future Upgrade

A useful later upgrade would be parsing `$GxGSA` sentences. GSV reports satellites in view, while GSA can identify which satellites are actively used in the fix. That would allow the UI to distinguish seen satellites from fix-used satellites.
