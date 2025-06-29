#include "WiFiController.h"
#include <Arduino.h>

// Static constants
const char *WiFiController::AP_NAME = "RO-Monitor-Setup";
const char *WiFiController::AP_PASSWORD = "setup123";

WiFiController::WiFiController() : currentStatus(WiFiStatus::DISCONNECTED),
                                   lastConnectionAttempt(0),
                                   lastStatusCheck(0),
                                   connectionAttempts(0),
                                   deviceNameParam(nullptr),
                                   ntpServerParam(nullptr),
                                   timezoneParam(nullptr)
{
}

WiFiController::~WiFiController()
{
    delete deviceNameParam;
    delete ntpServerParam;
    delete timezoneParam;
}

void WiFiController::begin()
{
    Serial.println("WiFiController: Initializing...");

    // Initialize preferences with a more specific namespace
    preferences.begin("wifi-settings", false);

    // Clear any corrupted data on first boot (optional safety check)
    size_t freeEntries = preferences.freeEntries();
    Serial.printf("WiFiController: NVS free entries: %zu\n", freeEntries);
    
    // Debug: Check if NVS is working
    Serial.printf("WiFiController: NVS partition size: %zu bytes\n", preferences.getBytesLength("configured"));
    
    // Test NVS write capability
    preferences.putString("test", "hello");
    String testRead = preferences.getString("test", "fail");
    Serial.printf("WiFiController: NVS test write/read: %s\n", testRead.c_str());

    // Load saved parameters
    loadSavedParameters();

    // Set up custom parameters
    setupCustomParameters();

    // Configure WiFiManager
    wifiManager.setDebugOutput(true);
    wifiManager.setSaveConfigCallback([this]()
                                      { this->saveCustomParameters(); });
    wifiManager.setAPCallback([this](WiFiManager *manager)
                              { this->handleConfigModeCallback(manager); });

    // Set timeouts
    wifiManager.setConfigPortalTimeout(300); // 5 minutes
    wifiManager.setConnectTimeout(30);       // 30 seconds

    // Try to connect with saved credentials
    if (hasCredentials())
    {
        Serial.println("WiFiController: Attempting to connect with saved credentials...");
        currentStatus = WiFiStatus::CONNECTING;

        // Try auto-connect first
        if (wifiManager.autoConnect(AP_NAME, AP_PASSWORD))
        {
            Serial.println("WiFiController: Connected successfully!");
            currentStatus = WiFiStatus::CONNECTED;
            onConnected();
        }
        else
        {
            Serial.println("WiFiController: Auto-connect failed, starting config portal...");
            currentStatus = WiFiStatus::CONFIG_MODE;
            onConfigModeStarted();
        }
    }
    else
    {
        Serial.println("WiFiController: No saved credentials, starting config portal...");
        startConfigPortal();
    }

    lastStatusCheck = millis();
}

void WiFiController::update()
{
    unsigned long now = millis();

    // Check status periodically
    if (now - lastStatusCheck >= STATUS_CHECK_INTERVAL)
    {
        WiFiStatus previousStatus = currentStatus;

        if (WiFi.status() == WL_CONNECTED)
        {
            if (currentStatus != WiFiStatus::CONNECTED)
            {
                currentStatus = WiFiStatus::CONNECTED;
                connectionAttempts = 0;
                onConnected();
            }
        }
        else
        {
            if (currentStatus == WiFiStatus::CONNECTED)
            {
                currentStatus = WiFiStatus::DISCONNECTED;
                onDisconnected();

                // Try to reconnect
                Serial.println("WiFiController: Connection lost, attempting to reconnect...");
                WiFi.reconnect();
                lastConnectionAttempt = now;
                connectionAttempts++;
            }
            else if (currentStatus == WiFiStatus::CONNECTING)
            {
                // Check if connection attempt timed out
                if (now - lastConnectionAttempt >= CONNECTION_TIMEOUT)
                {
                    connectionAttempts++;
                    if (connectionAttempts >= MAX_CONNECTION_ATTEMPTS)
                    {
                        Serial.println("WiFiController: Max connection attempts reached, starting config portal...");
                        startConfigPortal();
                    }
                    else
                    {
                        Serial.printf("WiFiController: Connection attempt %d failed, retrying...\n", connectionAttempts);
                        lastConnectionAttempt = now;
                    }
                }
            }
        }

        lastStatusCheck = now;
    }
}

void WiFiController::setupCustomParameters()
{
    // Device name parameter
    deviceNameParam = new WiFiManagerParameter(
        "device_name",
        "Device Name",
        deviceName.c_str(),
        32,
        "placeholder=\"RO Monitor\"");

    // NTP Server parameter
    ntpServerParam = new WiFiManagerParameter(
        "ntp_server",
        "NTP Server",
        ntpServer.c_str(),
        64,
        "placeholder=\"pool.ntp.org\"");

    // Timezone parameter
    timezoneParam = new WiFiManagerParameter(
        "timezone",
        "Timezone Offset (hours)",
        timezone.c_str(),
        8,
        "placeholder=\"0\" type=\"number\" min=\"-12\" max=\"12\"");

    // Add parameters to WiFiManager
    wifiManager.addParameter(deviceNameParam);
    wifiManager.addParameter(ntpServerParam);
    wifiManager.addParameter(timezoneParam);
}

void WiFiController::saveCustomParameters()
{
    Serial.println("WiFiController: Saving custom parameters...");

    // Get values from parameters
    deviceName = String(deviceNameParam->getValue());
    ntpServer = String(ntpServerParam->getValue());
    timezone = String(timezoneParam->getValue());

    // Set defaults if empty
    if (deviceName.length() == 0)
        deviceName = "RO Monitor";
    if (ntpServer.length() == 0)
        ntpServer = "pool.ntp.org";
    if (timezone.length() == 0)
        timezone = "0";

    // Save to preferences
    preferences.putString("device_name", deviceName);
    preferences.putString("ntp_server", ntpServer);
    preferences.putString("timezone", timezone);
    preferences.putBool("configured", true);

    Serial.printf("WiFiController: Saved - Device: %s, NTP: %s, TZ: %s\n",
                  deviceName.c_str(), ntpServer.c_str(), timezone.c_str());
}

void WiFiController::loadSavedParameters()
{
    deviceName = preferences.getString("device_name", "RO Monitor");
    ntpServer = preferences.getString("ntp_server", "pool.ntp.org");
    timezone = preferences.getString("timezone", "0");

    Serial.printf("WiFiController: Loaded - Device: %s, NTP: %s, TZ: %s\n",
                  deviceName.c_str(), ntpServer.c_str(), timezone.c_str());
}

bool WiFiController::shouldEnterConfigMode()
{
    // Enter config mode if no credentials or unable to connect
    return !hasCredentials() || (connectionAttempts >= MAX_CONNECTION_ATTEMPTS);
}

void WiFiController::handleConfigModeCallback(WiFiManager *myWiFiManager)
{
    Serial.println("WiFiController: Entered config mode");
    Serial.printf("WiFiController: Config AP: %s\n", AP_NAME);
    Serial.printf("WiFiController: Config IP: %s\n", WiFi.softAPIP().toString().c_str());

    currentStatus = WiFiStatus::CONFIG_MODE;
    onConfigModeStarted();
}

void WiFiController::startConfigPortal()
{
    Serial.println("WiFiController: Starting config portal...");
    currentStatus = WiFiStatus::CONFIG_MODE;

    // Start the config portal
    if (wifiManager.startConfigPortal(AP_NAME, AP_PASSWORD))
    {
        Serial.println("WiFiController: Config portal completed successfully");
        currentStatus = WiFiStatus::CONNECTED;
        onConnected();
    }
    else
    {
        Serial.println("WiFiController: Config portal failed or timed out");
        currentStatus = WiFiStatus::ERROR;
    }
}

void WiFiController::resetSettings()
{
    Serial.println("WiFiController: Resetting WiFi settings...");
    wifiManager.resetSettings();
    preferences.clear();

    // Reset parameters to defaults
    deviceName = "RO Monitor";
    ntpServer = "pool.ntp.org";
    timezone = "0";

    currentStatus = WiFiStatus::DISCONNECTED;
    connectionAttempts = 0;
}

String WiFiController::getIPAddress() const
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

String WiFiController::getStatusString() const
{
    switch (currentStatus)
    {
    case WiFiStatus::DISCONNECTED:
        return "Disconnected";
    case WiFiStatus::CONNECTING:
        return "Connecting...";
    case WiFiStatus::CONNECTED:
        return "Connected";
    case WiFiStatus::CONFIG_MODE:
        return "Setup Mode";
    case WiFiStatus::ERROR:
        return "Error";
    default:
        return "Unknown";
    }
}

bool WiFiController::hasCredentials() const
{
    // Check if we have saved WiFi credentials in NVS
    // WiFiManager will handle checking its own internal storage
    return const_cast<Preferences &>(preferences).getBool("configured", false);
}

unsigned long WiFiController::getUptime() const
{
    return millis();
}

void WiFiController::onConnected()
{
    Serial.printf("WiFiController: Connected to %s\n", WiFi.SSID().c_str());
    Serial.printf("WiFiController: IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("WiFiController: Signal Strength: %d dBm\n", WiFi.RSSI());
}

void WiFiController::onDisconnected()
{
    Serial.println("WiFiController: Disconnected from WiFi");
}

void WiFiController::onConfigModeStarted()
{
    Serial.println("WiFiController: Configuration mode started");
    Serial.printf("WiFiController: Connect to '%s' and go to http://%s\n",
                  AP_NAME, WiFi.softAPIP().toString().c_str());
}
