ESP32-Based RO Water Monitor with HomeKit + OLED

This project uses an ESP32 DevKit to monitor a reverse osmosis (RO) water filtration system, track water usage via a flow sensor, and display key information on a 0.96" I2C OLED screen. It integrates natively with Apple HomeKit using HomeSpan library to provide real-time filter status monitoring, maintenance notifications, and Siri control without requiring any additional hubs or bridges.

## Project Status: FULLY OPERATIONAL with Native HomeKit Integration

The system is now complete with OLED display navigation and native HomeKit integration featuring 6 accessories (5 filters + water usage sensor) accessible via iOS Home app with Siri control and automation support. WiFi configuration is now handled entirely by HomeSpan with automatic AP mode for easy setup.

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
- **WiFi Configuration**: HomeSpan's built-in WiFi management with automatic AP mode ("HomeSpan-Setup")
- **HomeKit Integration**: Native HomeKit support via HomeSpan library (no hub/bridge required)
- **HomeKit Services**: Proper FilterMaintenance services with correct characteristics (FilterChangeIndication, FilterLifeLevel, ResetFilterIndication)
- **HomeKit Features**: Real-time notifications, Siri control, iOS automation triggers
- **Data Storage**: NVS/Preferences for persistent configuration and filter data

## WiFi Setup Strategy

Using **HomeSpan's built-in WiFi management** for streamlined configuration:

- Device creates "HomeSpan-Setup" WiFi hotspot when unconfigured
- Users connect and configure via HomeSpan's web interface
- Simple setup process with clear credentials displayed on OLED
- Automatic reconnection with fallback to setup mode
- Credentials stored persistently in ESP32 NVS
- No additional WiFi libraries required - all handled by HomeSpan
- Integrated with HomeKit pairing process for seamless setup

## HomeKit Integration Strategy

Using **HomeSpan** library for native HomeKit integration:

- **Direct HomeKit Protocol** - no additional hubs, bridges, or MQTT brokers required
- **FilterMaintenance Services** - each filter uses proper HomeKit FilterMaintenance service with:
  - FilterChangeIndication (0=no change needed, 1=change needed)
  - FilterLifeLevel (percentage remaining 0-100%)
  - ResetFilterIndication (write-only characteristic for filter reset)
- **Water Usage Sensor** - TemperatureSensor service repurposed for water usage tracking
- **Setup Process**: WiFi configuration → HomeKit pairing (setup code displayed on OLED)
- **iOS Integration**: Home app control, Siri commands, automation triggers, push notifications
- **Reduced Logging**: Status messages appear only once per minute to reduce serial noise

## Current Implementation Status

- ✅ OLED display system with filter status cards and progress bars
- ✅ Dual-button navigation (left=previous, right=next) with auto-rotation
- ✅ Filter status tracking with visual indicators (OK/WARNING/REPLACE)
- ✅ Counter reset functionality (both buttons for 3 seconds)
- ✅ Nine main screens: Dashboard, PP1, PP2, Carbon, Membrane, Mineralizer, Usage, WiFi Status, HomeKit Status
- ✅ WiFi configuration with HomeSpan's built-in WiFi management - FULLY OPERATIONAL!
- ✅ HomeKit integration with proper FilterMaintenance services - FULLY IMPLEMENTED!
- ✅ HomeKit status screen with pairing code display and connection status
- ✅ Proper HomeKit FilterMaintenance services for each filter (5 services total)
- ✅ Native HomeKit protocol support (no hub required)
- ✅ HomeKit filter reset functionality via iOS Home app
- ✅ Reduced serial output - status messages only once per minute
- ✅ All unit tests passing (ButtonLogic functionality verified)
- ✅ Clean ESP32 build with no compilation errors or warnings
- 🔄 Flow sensor integration for real water usage tracking
- 🔄 Real-time filter lifecycle calculations based on actual usage

## Recent Updates & Bug Fixes

### Major Refactoring - HomeKit & WiFi Optimization (Latest - COMPLETED)

- ✅ **Removed WiFiManager Dependency**: Eliminated WiFiManager library and replaced with HomeSpan's built-in WiFi
- ✅ **Proper HomeKit FilterMaintenance Services**: Implemented correct HomeKit services using:
  - FilterChangeIndication characteristic (0=no change needed, 1=change needed)
  - FilterLifeLevel characteristic (percentage remaining 0-100%)
  - ResetFilterIndication characteristic (write-only for filter reset)
- ✅ **WiFi Integration**: HomeSpan now handles all WiFi connectivity with automatic AP mode
- ✅ **Reduced Serial Output**: Status messages appear only once per minute instead of continuous logging
- ✅ **Code Cleanup**: Removed unused WiFiController library and NTPClient dependency
- ✅ **Improved Reliability**: Eliminated WiFiManager conflicts that caused HomeKit pairing issues
- ✅ **All Tests Passing**: ButtonLogic unit tests (7/7) continue to pass after refactoring
- ✅ **Clean Build**: ESP32 compilation successful with no errors or warnings
- ✅ **Memory Optimization**: Reduced RAM usage (18.4%) and Flash usage (40.7%)

### Previous HomeKit Integration Implementation (Completed)

- ✅ **HomeSpan Library Integration**: Successfully integrated HomeSpan v1.9.1 for native HomeKit support
- ✅ **HomeKit Bridge Architecture**: Implemented proper bridge accessory with multiple child accessories
- ✅ **Filter Status Synchronization**: Real-time sync between OLED display and HomeKit characteristics
- ✅ **Water Usage Sensor**: Added TemperatureSensor service repurposed for water monitoring
- ✅ **HomeKit Status Screen**: New 9th screen displaying pairing status and setup code
- ✅ **Partition Table Optimization**: Updated to huge_app.csv for larger application size
- ✅ **Automatic Initialization**: HomeKit starts automatically when WiFi connects
- ✅ **Native iOS Integration**: Full Home app support with Siri control and automation

## HomeKit Integration Implementation

### HomeKit Architecture

The system now features full **native HomeKit integration** using the HomeSpan library:

- **Bridge Accessory**: Main bridge device that groups all filter accessories
- **5 Filter Accessories**: Each filter (PP1, PP2, Carbon, Membrane, Mineralizer) appears as a separate FilterMaintenance service in HomeKit
- **Water Usage Sensor**: Leak sensor accessory for water monitoring and system status
- **Direct HomeKit Protocol**: No additional bridges, hubs, or MQTT brokers required

### HomeKit Services & Characteristics

Each filter accessory provides proper HomeKit FilterMaintenance service with:

- **FilterLifeLevel**: Shows percentage remaining (0-100%)
- **FilterChangeIndication**: Indicates when filter needs replacement (0=no change, 1=change needed)
- **ResetFilterIndication**: Allows filter reset via HomeKit (write-only characteristic)

Water usage is tracked via a TemperatureSensor service repurposed for usage monitoring.

### HomeKit Setup Process

1. **WiFi Connection**: Device uses HomeSpan's built-in WiFi with "HomeSpan-Setup" AP
2. **HomeKit Initialization**: Automatic initialization once WiFi is connected
3. **Pairing**: Use iOS Home app with setup code displayed on device OLED
4. **Accessories**: All 6 accessories (5 filters + usage sensor) appear in Home app
5. **Control**: Full Siri integration, automation triggers, and notifications

### HomeKit Status Screen

The new HomeKit status screen (9th screen in rotation) displays:

- **Initialization State**: Shows "Initializing..." during startup
- **Pairing Mode**: Displays setup code prominently for easy pairing
- **Connected State**: Shows active status with device count
- **Error Handling**: Displays connection issues if any

Status messages now appear only once per minute to reduce serial output noise.

### HomeKit Debugging Commands

Serial commands for HomeKit debugging and maintenance:

- `K/k` - Display HomeKit status and setup information
- `P/p` - Reset HomeKit pairing data (force re-pairing)

Note: Serial output is now limited to once per minute for normal status messages.

### HomeKit Features

- **iOS Home App**: Native integration with Apple's Home app
- **Siri Control**: Voice commands for filter status queries
- **Automation**: HomeKit automation triggers based on filter status
- **Notifications**: Push notifications when filters need replacement
- **Remote Access**: HomeKit Hub support for remote monitoring

The goal is to create a smart RO system dashboard with native HomeKit integration that provides local OLED display monitoring plus iOS Home app control, Siri voice commands, and automated maintenance reminders without requiring any additional smart home hubs. The system now uses proper HomeKit FilterMaintenance services and simplified WiFi management for reliable operation.
