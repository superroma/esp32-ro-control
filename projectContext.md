ESP32-Based RO Water Monitor with HomeKit + OLED

This project uses an ESP32 DevKit to monitor a reverse osmosis (RO) water filtration system, track water usage via a flow sensor, and display key information on a 0.96" I2C OLED screen. It integrates natively with Apple HomeKit using HomeSpan library to provide real-time filter status monitoring, maintenance notifications, and Siri control without requiring any additional hubs or bridges.

## Hardware Components

- ESP-WROOM-32 DevKit (Wi-Fi microcontroller, 520KB RAM, 4MB Flash)
- SSD1306 OLED display (128x64, I2C) - pins 21 (SDA), 22 (SCL)
- Two push buttons for navigation:
  - Left button (GPIO 0) - Previous screen
  - Right button (GPIO 2) - Next screen
- Flow meter (e.g. YF-S201) for water usage tracking (planned)
- Optional: TDS sensors for water quality monitoring

## Software Architecture

- **Display System**: Multi-screen interface with 7 normal screens (dashboard + individual filter screens + usage)
- **Navigation System**: Dual-button control with left/right navigation and auto-rotation
- **Counter Reset Feature**: Hold both buttons for 3 seconds to access special reset screen
- **Filter Monitoring**: Tracks 5 filters (PP1, PP2, Carbon, Membrane, Mineralizer) with status indicators
- **WiFi Configuration**: WiFiManager library for easy setup via temporary AP and web interface
- **HomeKit Integration**: Native HomeKit support via HomeSpan library (no hub/bridge required)
- **HomeKit Accessories**: Custom filter maintenance sensors, water usage tracking, and leak detection
- **HomeKit Features**: Real-time notifications, Siri control, iOS automation triggers
- **Data Storage**: NVS/Preferences for persistent configuration and filter data

## WiFi Setup Strategy

Using **WiFiManager** library for user-friendly configuration:

- Device creates "RO-Monitor-Setup" WiFi hotspot when unconfigured
- Users connect and configure via web interface (192.168.4.1)
- Supports custom parameters: device name, filter thresholds, HomeKit setup preferences
- Automatic reconnection with fallback to setup mode
- Credentials stored persistently in ESP32 NVS

## HomeKit Integration Strategy

Using **HomeSpan** library for native HomeKit integration:

- **Direct HomeKit Protocol** - no additional hubs, bridges, or MQTT brokers required
- **Custom HomeKit Accessories** - each filter as FilterMaintenance service with percentage remaining
- **HomeKit Services**: Water usage sensor, leak detection, system status notifications
- **Setup Process**: WiFi configuration → HomeKit pairing (setup code displayed on OLED)
- **iOS Integration**: Home app control, Siri commands, automation triggers, push notifications

## Current Implementation Status

- ✅ OLED display system with filter status cards and progress bars
- ✅ Dual-button navigation (left=previous, right=next) with auto-rotation
- ✅ Filter status tracking with visual indicators (OK/WARNING/REPLACE)
- ✅ Counter reset functionality (both buttons for 3 seconds)
- ✅ Eight main screens: Dashboard, PP1, PP2, Carbon, Membrane, Mineralizer, Usage, WiFi Status
- ✅ WiFi configuration with WiFiManager - COMPLETED!
- ✅ WiFi status screen showing connection info and setup instructions
- ✅ WiFi settings reset functionality (both buttons on WiFi screen)
- 🔄 HomeKit pairing screen and setup code display
- 🔄 HomeSpan integration with custom filter accessories
- 🔄 Flow sensor integration for real water usage tracking
- 🔄 Real-time filter lifecycle calculations based on actual usage

The goal is to create a smart RO system dashboard with native HomeKit integration that provides local OLED display monitoring plus iOS Home app control, Siri voice commands, and automated maintenance reminders without requiring any additional smart home hubs.
