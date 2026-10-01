# esp32-modular-bms
a BMS on ESP32 that spots the weakest cell, tracks voltage imbalance over time, and adapts its alarm limits to the battery's charge level. If something goes wrong, it cuts the load automatically.
# Modular Battery Management System (ESP32 + Wokwi)

Modular BMS on ESP32 with a scalable cell count, adaptive imbalance
threshold, imbalance trend tracking, and relay protection.

▶ run it live <https://wokwi.com/projects/476677840515067905>

## Features
- Finds the weakest and strongest cell
- Calculates voltage imbalance and tracks whether it is rising or falling
- Adaptive threshold based on State of Charge (0.10 V full to 0.30 V empty)
- Relay cutoff, LED, buzzer and LCD alerts
- Cell count set by one constant: `#define NUM_CELLS`
- All data in one `BmsData` struct filled by `analyzeBms()` for reuse

## Hardware (Wokwi)
ESP32, 4 potentiometers (GPIO 34, 35, 32, 33), I2C LCD (21, 22),
red/green/yellow LEDs (2, 4, 5), buzzer (18), relay (19).

## Run
1. Open the Wokwi link, or create an ESP32 project and paste
   `sketch.ino` and `diagram.json`.
2. Add the `LiquidCrystal I2C` library.
3. Press Play and turn the potentiometers.

## Scaling
Memory grows linearly (about 56 B at 4 cells, 152 B at 16 cells) and the
analysis is O(N). See the report for details.

## demo video
<https://lnkd.in/p/gXr34v7B>

## screenshot
<img width="857" height="701" alt="Screenshot 2026-10-01 171449" src="https://github.com/user-attachments/assets/dcb88e70-5c89-4a5d-95fb-3860b5905d39" />


