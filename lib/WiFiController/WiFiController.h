#pragma once

#include <WiFi.h>
#include <WiFiManager.h>
#include <Preferences.h>

enum class WiFiStatus
{
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    CONFIG_MODE,
    ERROR
};

class WiFiController
{
private:
    WiFiManager wifiManager;
    Preferences preferences;
    WiFiStatus currentStatus;
    unsigned long lastConnectionAttempt;
    unsigned long lastStatusCheck;
    int connectionAttempts;

    // Configuration constants
    static const unsigned long CONNECTION_TIMEOUT = 30000;   // 30 seconds
    static const unsigned long STATUS_CHECK_INTERVAL = 5000; // 5 seconds
    static const int MAX_CONNECTION_ATTEMPTS = 3;
    static const char *AP_NAME;
    static const char *AP_PASSWORD;

    // Custom parameters for WiFiManager
    WiFiManagerParameter *deviceNameParam;
    WiFiManagerParameter *ntpServerParam;
    WiFiManagerParameter *timezoneParam;

    // Device configuration
    String deviceName;
    String ntpServer;
    String timezone;

    void setupCustomParameters();
    void saveCustomParameters();
    void loadSavedParameters();
    bool shouldEnterConfigMode();
    void handleConfigModeCallback(WiFiManager *myWiFiManager);

public:
    WiFiController();
    ~WiFiController();

    // Main control methods
    void begin();
    void update();
    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
    WiFiStatus getStatus() const { return currentStatus; }

    // Configuration methods
    void startConfigPortal();
    void resetSettings();
    String getDeviceName() const { return deviceName; }
    String getSSID() const { return WiFi.SSID(); }
    String getIPAddress() const;
    int getRSSI() const { return WiFi.RSSI(); }
    String getAPName() const { return AP_NAME; }
    String getAPPassword() const { return AP_PASSWORD; }
    String getAPIP() const { return WiFi.softAPIP().toString(); }

    // Status information
    String getStatusString() const;
    bool hasCredentials() const;
    unsigned long getUptime() const;

    // Event callbacks
    void onConnected();
    void onDisconnected();
    void onConfigModeStarted();
};
