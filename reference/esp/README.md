# ESP32 Counter with SSD1306 OLED Display

A simple PlatformIO project for ESP-WROOM-32 DevKit that displays a counting number on an SSD1306 OLED display. The counter value is stored in non-volatile storage and persists through resets and code uploads.

## Hardware Requirements

- ESP-WROOM-32 DevKit
- SSD1306 OLED Display (128x64, I2C)
- Jumper wires

## Wiring

| OLED Pin | ESP32 Pin | Description |
| -------- | --------- | ----------- |
| VCC      | 3.3V      | Power       |
| GND      | GND       | Ground      |
| SDA      | GPIO 21   | I2C Data    |
| SCL      | GPIO 22   | I2C Clock   |

## Features

- Counts from 0 incrementing every second
- Displays count on OLED screen with additional information
- Stores counter value in ESP32's NVS (Non-Volatile Storage)
- Counter persists through:
  - Power cycles
  - Resets
  - Code uploads
- Serial output for debugging

## Getting Started

1. Connect the OLED display as per the wiring diagram above
2. Open this project in PlatformIO
3. Build and upload to your ESP32
4. Open Serial Monitor to see debug output

## Display Layout

The OLED shows:

- Title: "ESP32 Counter"
- Current count (large text)
- System uptime in seconds
- "Persistent Storage" indicator

## Troubleshooting

- If display doesn't work, check I2C address (default: 0x3C)
- Verify wiring connections
- Check Serial Monitor for error messages
- Ensure OLED display is 128x64 I2C variant

## Libraries Used

- Adafruit SSD1306
- Adafruit GFX Library
- Adafruit BusIO
- ESP32 Preferences (built-in)
