# GitHub Copilot Instructions

applyTo: "\*\*"

## Project Overview

This project is an ESP32-S3-based embedded system developed using PlatformIO and Visual Studio Code. It runs on an M5Stack Atom S3 (ESP32-S3) board. The project is for a reverse osmosis (RO) water filtration system that monitors water flow and tracks filter usage. It uses the YF-S402 flow sensor to measure water consumption, counts pulses via GPIO interrupts, and exposes metrics over Wi-Fi using the Matter protocol for Apple HomeKit integration.

## Board and Framework

- Target: M5Stack Atom S3 (ESP32-S3)
- PlatformIO framework: Arduino or ESP-IDF depending on project configuration
- GPIO33 is used to read flow sensor pulses (with pull-up and interrupt)
- GPIO38 (G38) is used for flow sensor input
- RGB LED is used for status indication
- Persistent state (total liters) is stored in NVS
- Screen cycles through UI views

## Wi-Fi Settings

- Hardcoded SSID: `P3`
- Hardcoded password: `chrysler34`

## Replaceable Parts in RO System

Each part has its own max liter threshold:

- PP1
- PP2
- Membrane
- Carbon
- Mineralizer

Each part shows a percentage left (100% = new, 0% = needs replacement).

## UI Screens

The system cycles through 6 screens:

1. PP1 percentage remaining
2. PP2 percentage remaining
3. Membrane percentage remaining
4. Carbon percentage remaining
5. Mineralizer percentage remaining
6. Total liters filtered

## Project Phases

1. **Phase 1 – Hello World:** Display text on screen and write to terminal.
2. **Phase 2 – Wi-Fi:** Connect to hardcoded Wi-Fi; verify connection.
3. **Phase 3 – UI Screens:** Cycle through parts with random percentage data and total liters.
4. **Phase 4 – HomeKit/Matter:** Integrate with Matter to expose attributes via HomeKit.
5. **Phase 5 – Flow Meter:** Connect and calibrate flow sensor input from GPIO38.

## Architecture

- ISR captures flow sensor pulses and increments a counter
- Main task polls pulse count every second, calculates flow rate (L/min)
- Total liters are accumulated and stored
- Matter cluster is updated with flow rate and filter status

## Coding Conventions

- Use `camelCase` for variables, `PascalCase` for class names
- Use `constexpr` or `#define` for constants like pin numbers and thresholds
- Use `Serial.printf` or `ESP_LOGI/E/W/E` macros for debug output
- Prefer non-blocking logic; no `delay()` in main tasks
- Break logic into reusable, testable functions when possible

## Matter Integration

- Flow Measurement cluster (m³/h × 10)
- Optional Occupancy cluster for end-of-life indication (boolean flag)
- Expose attributes via esp-matter framework
- Setup Code and QR for onboarding (Wi-Fi commissioning)

## Testing and Debugging

- Use logic analyzer or Serial output to verify pulse timing
- Use Fritzing or schematic view to verify physical wiring
- Ensure GPIO38 input does not exceed 3.3V (resistor divider required)

## Copilot Usage Notes

- Suggest boilerplate for GPIO setup, ISR registration, NVS read/write, and Matter attribute handling
- Assist with async event handling or scheduler logic
- Provide snippets for status LED behavior based on flow state or filter age
- Support for cycling UI, formatting percentage displays, and updating screen at regular intervals
