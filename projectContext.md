ESP32-Based RO Water Monitor with HomeKit + OLED

This project uses an ESP32 DevKit to monitor a reverse osmosis (RO) water filtration system, track water usage via a flow sensor, and display key information on a 0.96" I2C OLED screen. It integrates natively with Apple HomeKit using HomeSpan library to provide real-time filter status monitoring, maintenance notifications, and Siri control without requiring any additional hubs or bridges.

## Project Status: FULLY OPERATIONAL with Native HomeKit Integration

The system is now complete with full WiFi configuration, OLED display navigation, and native HomeKit integration featuring 6 accessories (5 filters + water usage sensor) accessible via iOS Home app with Siri control and automation support.

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
- Enhanced setup screen displays clear connection instructions on OLED:
  - Network name: "RO-Monitor-Setup"
  - Password: "setup123"
  - Setup IP: "192.168.4.1"
- Supports custom parameters: device name, NTP server, timezone offset
- Automatic reconnection with fallback to setup mode
- Credentials stored persistently in ESP32 NVS
- WiFi status screen shows current connection details when connected
- Reset functionality available through button combination on WiFi screen

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
- ✅ Nine main screens: Dashboard, PP1, PP2, Carbon, Membrane, Mineralizer, Usage, WiFi Status, HomeKit Status
- ✅ WiFi configuration with WiFiManager - FULLY OPERATIONAL!
- ✅ WiFi status screen showing connection info and setup instructions
- ✅ WiFi settings reset functionality (both buttons on WiFi screen)
- ✅ Enhanced WiFi setup screen with clear connection instructions (SSID, password, IP)
- ✅ Serial communication debugging and monitoring (115200 baud)
- ✅ WiFi configuration portal tested and working (connects to network successfully)
- ✅ HomeKit integration with HomeSpan library - FULLY IMPLEMENTED!
- ✅ HomeKit status screen with pairing code display and connection status
- ✅ Custom HomeKit accessories for each filter (5 FilterMaintenance services)
- ✅ Native HomeKit protocol support (no hub required)
- ✅ HomeKit setup code: 466-37-726 (default HomeSpan code)
- ✅ HomeKit filter reset functionality via iOS Home app
- ✅ HomeKit debug commands and status monitoring
- 🔄 Flow sensor integration for real water usage tracking
- 🔄 Real-time filter lifecycle calculations based on actual usage

## Recent Updates & Bug Fixes

### HomeKit Integration (Latest - MAJOR UPDATE + CONNECTION FIX)

- ✅ **HomeSpan Library Integration**: Successfully integrated HomeSpan v1.9.1 for native HomeKit support
- ✅ **Custom HomeKit Accessories**: Created 5 individual FilterMaintenance accessories for each filter
- ✅ **HomeKit Bridge Architecture**: Implemented proper bridge accessory with multiple child accessories
- ✅ **Filter Status Synchronization**: Real-time sync between OLED display and HomeKit characteristics
- ✅ **HomeKit Reset Functionality**: Filters can be reset to 100% via HomeKit interface
- ✅ **Water Usage Sensor**: Added LeakSensor service for water monitoring integration
- ✅ **HomeKit Status Screen**: New 9th screen displaying pairing status and setup code
- ✅ **Setup Code Display**: Prominently shows setup code (466-37-726) for easy pairing
- ✅ **HomeKit Debug Commands**: Added K/k and P/p serial commands for debugging
- ✅ **Partition Table Optimization**: Updated to huge_app.csv for larger application size
- ✅ **Automatic Initialization**: HomeKit starts automatically when WiFi connects
- ✅ **Native iOS Integration**: Full Home app support with Siri control and automation
- ✅ **Connection Issue Fix**: Improved accessory initialization with proper error handling and logging
- ✅ **Enhanced Debugging**: Added detailed logging at LogLevel 2 for connection troubleshooting
- ✅ **Stable Home App Connection**: Fixed "spinning" issue with better accessory response handling

### WiFi Configuration Enhancement (Completed)

- ✅ **Enhanced Setup Screen**: Initial WiFi setup screen now displays comprehensive connection instructions
- ✅ **Clear User Instructions**: Shows network name, password, and IP address on OLED during setup
- ✅ **Improved User Experience**: Users can see connection details immediately upon device startup
- ✅ **Dynamic Information Display**: WiFi status screen shows both setup mode and connected state information

### Serial Communication Fix (Resolved)

- ✅ **Baud Rate Issue Resolved**: Fixed garbled serial output (`�x␀�x�x␀�x...`)
- ✅ **Correct Serial Configuration**: Confirmed 115200 baud rate matching between code and PlatformIO config
- ✅ **Serial Monitoring Working**: PlatformIO monitor command now works correctly
- ✅ **Debug Output Functional**: WiFi connection process visible through serial monitor

### WiFi Connection Testing (Verified)

- ✅ **Access Point Creation**: ESP32 successfully creates "RO-Monitor-Setup" hotspot
- ✅ **Web Portal Functional**: Configuration portal accessible at 192.168.4.1
- ✅ **Network Connection**: Successfully connects to home WiFi network
- ✅ **Parameter Persistence**: Device name, NTP server, and timezone settings saved correctly
- ✅ **IP Assignment**: Receives and displays local network IP address (192.168.1.123)
- ✅ **Signal Monitoring**: RSSI signal strength monitoring functional (-86 dBm observed)

## HomeKit Integration Implementation

### HomeKit Architecture

The system now features full **native HomeKit integration** using the HomeSpan library:

- **Bridge Accessory**: Main bridge device that groups all filter accessories
- **5 Filter Accessories**: Each filter (PP1, PP2, Carbon, Membrane, Mineralizer) appears as a separate FilterMaintenance service in HomeKit
- **Water Usage Sensor**: Leak sensor accessory for water monitoring and system status
- **Direct HomeKit Protocol**: No additional bridges, hubs, or MQTT brokers required

### HomeKit Services & Characteristics

Each filter accessory provides:

- **FilterLifeLevel**: Shows percentage remaining (0-100%)
- **FilterChangeIndication**: Indicates when filter needs replacement
- **ResetFilterIndication**: Allows filter reset via HomeKit (sets to 100%)

### HomeKit Setup Process

1. **WiFi Connection**: Device must be connected to WiFi first
2. **HomeKit Initialization**: Automatic initialization once WiFi is connected
3. **Pairing**: Use iOS Home app with setup code **466-37-726**
4. **Accessories**: All 6 accessories (5 filters + usage sensor) appear in Home app
5. **Control**: Full Siri integration, automation triggers, and notifications

### HomeKit Status Screen

The new HomeKit status screen (9th screen in rotation) displays:

- **Initialization State**: Shows "Initializing..." during startup
- **Pairing Mode**: Displays setup code prominently for easy pairing
- **Connected State**: Shows active status with device count
- **Error Handling**: Displays connection issues if any

### HomeKit Debugging Commands

New serial commands for HomeKit debugging:

- `K/k` - Display HomeKit status and setup information
- `P/p` - Reset HomeKit pairing data (force re-pairing)

### HomeKit Features

- **iOS Home App**: Native integration with Apple's Home app
- **Siri Control**: Voice commands for filter status queries
- **Automation**: HomeKit automation triggers based on filter status
- **Notifications**: Push notifications when filters need replacement
- **Remote Access**: HomeKit Hub support for remote monitoring

The goal is to create a smart RO system dashboard with native HomeKit integration that provides local OLED display monitoring plus iOS Home app control, Siri voice commands, and automated maintenance reminders without requiring any additional smart home hubs.
