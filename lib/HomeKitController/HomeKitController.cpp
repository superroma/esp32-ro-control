#include "HomeKitController.h"

// Global pointer to the HomeKit controller for callback access
static HomeKitController *globalHomeKitController = nullptr;

// HomeKit pairing callback function
void homeKitPairingCallback(bool isPaired)
{
    Serial.printf("HomeKit: Pairing callback - isPaired: %s\n", isPaired ? "true" : "false");
    if (globalHomeKitController)
    {
        globalHomeKitController->onPairingComplete(isPaired);
    }
}

// Forward declaration for filter status enum
enum FilterStatus
{
    STATUS_OK,      // >20%
    STATUS_WARNING, // 10-20%
    STATUS_REPLACE  // <10%
};

// Include the filter structure definition
struct FilterInfo
{
    String name;
    String shortName;
    int percentage;
    FilterStatus status;
    String timeLeft;
};

// Filter maintenance implementation using proper HomeKit FilterMaintenance service
DEV_FilterMaintenance::DEV_FilterMaintenance(FilterInfo *filter, int index) : Service::FilterMaintenance()
{
    filterRef = filter;
    filterIndex = index;

    // Initialize FilterMaintenance characteristics
    filterChangeIndication = new Characteristic::FilterChangeIndication(
        (filter->status == STATUS_REPLACE) ? 1 : 0 // 1=CHANGE_NEEDED, 0=NO_CHANGE_NEEDED
    );

    filterLifeLevel = new Characteristic::FilterLifeLevel(filter->percentage); // 0-100% remaining
    resetFilterIndication = new Characteristic::ResetFilterIndication();       // Write-only for reset

    // Only log filter creation during initial setup, not for every filter
    if (index == 0)
    {
        Serial.println("HomeKit: Creating 5 filter maintenance services...");
    }
    if (index == 4)
    {
        Serial.println("HomeKit: All filter maintenance services created successfully");
    }
}

void DEV_FilterMaintenance::loop()
{
    // Update filter status every 10 seconds
    if (filterLifeLevel->timeVal() > 10000)
    {
        updateFromFilter();

        // Log significant changes
        static int lastReportedPercentage[5] = {-1, -1, -1, -1, -1};
        if (abs(filterRef->percentage - lastReportedPercentage[filterIndex]) >= 5)
        {
            Serial.printf("HomeKit: Filter %d (%s) updated to %d%% - %s\n",
                          filterIndex + 1, filterRef->name.c_str(), filterRef->percentage,
                          (filterRef->status == STATUS_REPLACE) ? "CHANGE NEEDED" : "OK");
            lastReportedPercentage[filterIndex] = filterRef->percentage;
        }
    }
}

boolean DEV_FilterMaintenance::update()
{
    // Handle filter reset command from HomeKit
    if (resetFilterIndication->updated())
    {
        if (resetFilterIndication->getNewVal() == 1)
        {
            // Reset filter to 100%
            filterRef->percentage = 100;
            filterRef->status = STATUS_OK;
            filterRef->timeLeft = "6 months";

            // Update characteristics immediately
            filterLifeLevel->setVal(100);
            filterChangeIndication->setVal(0); // NO_CHANGE_NEEDED

            Serial.printf("HomeKit: Filter %d (%s) reset to 100%% via HomeKit\n",
                          filterIndex + 1, filterRef->name.c_str());

            return true; // Signal successful handling
        }
    }

    return true; // Always return true for proper operation
}

void DEV_FilterMaintenance::updateFromFilter()
{
    if (filterRef)
    {
        // Update filter life level
        filterLifeLevel->setVal(filterRef->percentage);

        // Update change indication based on filter status
        int changeNeeded = (filterRef->status == STATUS_REPLACE) ? 1 : 0;
        filterChangeIndication->setVal(changeNeeded);
    }
}

// Water usage sensor implementation - uses temperature sensor to report water usage
DEV_WaterUsageSensor::DEV_WaterUsageSensor(unsigned int *waterUsage) : Service::TemperatureSensor()
{
    waterUsageRef = waterUsage;

    // Use temperature to represent water usage (scaled down by 10 to fit in reasonable temperature range)
    // 1000 liters = 100°C, 500 liters = 50°C etc.
    float scaledUsage = (*waterUsage) / 10.0f;
    temperature = new Characteristic::CurrentTemperature(scaledUsage);
    temperature->setRange(0, 500); // 0-5000 liters range

    Serial.println("HomeKit: Water usage sensor created");
}

void DEV_WaterUsageSensor::loop()
{
    // Update water usage every 30 seconds
    if (temperature->timeVal() > 30000)
    {
        float scaledUsage = (*waterUsageRef) / 10.0f;
        temperature->setVal(scaledUsage);

        // Log significant changes
        static unsigned int lastReportedUsage = 0;
        if (abs((int)(*waterUsageRef) - (int)lastReportedUsage) >= 50)
        {
            Serial.printf("HomeKit: Water usage updated to %d liters\n", *waterUsageRef);
            lastReportedUsage = *waterUsageRef;
        }
    }
}

void DEV_WaterUsageSensor::updateFromUsage()
{
    if (waterUsageRef)
    {
        float scaledUsage = (*waterUsageRef) / 10.0f;
        temperature->setVal(scaledUsage);
    }
}

HomeKitController::HomeKitController()
{
    status = HOMEKIT_NOT_INITIALIZED;
    initialized = false;
    setupCode = "466-37-726"; // Default setup code
    lastUpdate = 0;

    // Initialize service pointers
    for (int i = 0; i < 5; i++)
    {
        filterMaintenanceServices[i] = nullptr;
    }
    waterUsageSensor = nullptr;

    // Set global pointer for callback access
    globalHomeKitController = this;
}

void HomeKitController::begin(FilterInfo filters[5], unsigned int *waterUsage)
{
    if (initialized)
    {
        Serial.println("HomeKit: Already initialized, skipping...");
        return;
    }

    Serial.println("HomeKit: ========== INITIALIZING HOMESPAN ==========");

    // Initialize preferences for HomeKit data storage
    prefs.begin("homekit", false);

    // Set up HomeSpan with clean configuration
    // Set WiFi hostname before HomeSpan initialization (no spaces, DNS-friendly)
    WiFi.setHostname("RO-Monitor-Bridge");

    try
    {
        // Simple HomeSpan initialization - HomeSpan will manage WiFi
        homeSpan.begin(Category::Bridges, "RO Monitor Bridge");
        Serial.println("HomeKit: HomeSpan initialized successfully");

        // Enable auto-start Access Point (HomeSpan uses default credentials)
        homeSpan.enableAutoStartAP();
        Serial.println("HomeKit: Access Point enabled with default credentials");
    }
    catch (...)
    {
        Serial.println("HomeKit: ERROR - HomeSpan.begin() failed!");
        status = HOMEKIT_ERROR;
        return;
    }

    // Set minimal logging to reduce serial output (0=minimal, 1=normal, 2=verbose)
    Serial.println("HomeKit: Setting log level to minimal (0) to reduce output...");
    homeSpan.setLogLevel(0);

    // Use the default setup code
    setupCode = "466-37-726"; // Default HomeSpan setup code
    Serial.printf("HomeKit: Setup code: %s\n", setupCode.c_str());
    Serial.println("HomeKit: WiFi Configuration:");
    Serial.println("HomeKit: - Type 'W' in serial monitor for manual WiFi setup");
    Serial.println("HomeKit: - Or connect to HomeSpan's default AP for web setup");

    // Create bridge accessory (required for multiple accessories)
    new SpanAccessory();
    new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Manufacturer("DIY Electronics");
    new Characteristic::SerialNumber("RO001");
    new Characteristic::Model("ESP32-RO-v1");
    new Characteristic::FirmwareRevision("1.0.0");

    // Create filter maintenance accessories
    const char *filterNames[] = {"PP1 Filter", "PP2 Filter", "Carbon Filter", "RO Membrane", "Mineralizer"};

    for (int i = 0; i < 5; i++)
    {
        // Create a new accessory for each filter
        new SpanAccessory();
        new Service::AccessoryInformation();
        new Characteristic::Identify();
        new Characteristic::Manufacturer("DIY Electronics");
        new Characteristic::SerialNumber(("FILTER" + String(i + 1)).c_str());
        new Characteristic::Model("RO Filter");
        new Characteristic::Name(filterNames[i]);
        new Characteristic::FirmwareRevision("1.0.0");

        // Add FilterMaintenance service with proper HomeKit characteristics
        filterMaintenanceServices[i] = new DEV_FilterMaintenance(&filters[i], i);

        // Small delay to ensure proper initialization
        delay(10);
    }

    // Create water usage sensor accessory
    new SpanAccessory();
    new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Manufacturer("DIY Electronics");
    new Characteristic::SerialNumber("USAGE001");
    new Characteristic::Model("Water Usage Sensor");
    new Characteristic::Name("Water Usage");
    new Characteristic::FirmwareRevision("1.0.0");

    // Add water usage sensor service (using temperature to represent usage)
    waterUsageSensor = new DEV_WaterUsageSensor(waterUsage);

    // Final initialization
    initialized = true;
    status = HOMEKIT_WAITING_FOR_PAIRING;

    Serial.println("HomeKit: ========== READY FOR PAIRING ==========");
    Serial.printf("HomeKit: Setup code: %s | Device: RO Monitor Bridge\n", setupCode.c_str());
    Serial.printf("HomeKit: Services: 6 total (5 filter maintenance + 1 water usage sensor)\n");
    Serial.println("HomeKit: Look for 'RO Monitor Bridge' in iOS Home app");
    Serial.println("HomeKit: Filter status shown as FilterChangeIndication & FilterLifeLevel");
    Serial.println("HomeKit: Water usage shown as temperature, filters support reset via HomeKit");
    Serial.println("HomeKit: ============================================");
    ;
}

void HomeKitController::update()
{
    if (!initialized)
    {
        return;
    }

    // Update HomeSpan - this is critical and should be called frequently
    homeSpan.poll();

    // Check pairing status using HomeSpan's pairing callbacks and status
    static bool wasPaired = false;
    static unsigned long lastStatusCheck = 0;
    static unsigned long lastConnectionLog = 0;

    // Check pairing status every 30 seconds (reduced from 2 seconds)
    if (millis() - lastStatusCheck > 30000)
    {
        // For HomeSpan 1.9.1, we need to monitor the serial output or use callbacks
        // For now, we'll use a simpler status tracking approach

        if (status == HOMEKIT_NOT_INITIALIZED)
        {
            status = HOMEKIT_WAITING_FOR_PAIRING;
        }

        lastStatusCheck = millis();
    }

    // Log connection status every 5 minutes (reduced from 15 seconds)
    if (millis() - lastConnectionLog > 300000)
    {
        Serial.printf("HomeKit: Status: %s, Setup Code: %s\n",
                      getStatusString().c_str(),
                      setupCode.c_str());

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.printf("HomeKit: WiFi IP: %s, mDNS: %s.local\n",
                          WiFi.localIP().toString().c_str(),
                          WiFi.getHostname());
        }
        else
        {
            Serial.println("HomeKit: WARNING - WiFi disconnected!");
        }

        lastConnectionLog = millis();
    }

    // Periodically update services (the services handle their own timing in their loop() methods)
    // This is just for fallback manual updates if needed
    if (millis() - lastUpdate > updateInterval)
    {
        for (int i = 0; i < 5; i++)
        {
            if (filterMaintenanceServices[i])
            {
                filterMaintenanceServices[i]->updateFromFilter();
            }
        }
        if (waterUsageSensor)
        {
            waterUsageSensor->updateFromUsage();
        }
        lastUpdate = millis();
    }
}

HomeKitStatus HomeKitController::getStatus()
{
    return status;
}

String HomeKitController::getSetupCode()
{
    return setupCode;
}

bool HomeKitController::isPaired()
{
    if (!initialized)
    {
        return false;
    }

    // In HomeSpan 1.9.1, we don't have direct access to controller count
    // We'll use a simple status-based approach
    return (status == HOMEKIT_PAIRED || status == HOMEKIT_RUNNING);
}

void HomeKitController::updateSensors(FilterInfo filters[5], unsigned int waterUsage)
{
    if (!initialized)
    {
        return;
    }

    // Update filter maintenance services
    for (int i = 0; i < 5; i++)
    {
        if (filterMaintenanceServices[i])
        {
            filterMaintenanceServices[i]->updateFromFilter();
        }
    }

    // Update water usage sensor
    if (waterUsageSensor)
    {
        waterUsageSensor->updateFromUsage();
    }
}

String HomeKitController::getStatusString()
{
    switch (status)
    {
    case HOMEKIT_NOT_INITIALIZED:
        return "Not Initialized";
    case HOMEKIT_WAITING_FOR_PAIRING:
        return "Waiting for Pair";
    case HOMEKIT_PAIRED:
        return "Paired";
    case HOMEKIT_RUNNING:
        return "Connected";
    case HOMEKIT_ERROR:
        return "Error";
    default:
        return "Unknown";
    }
}

void HomeKitController::resetPairing()
{
    if (!initialized)
    {
        Serial.println("HomeKit: Cannot reset - not initialized");
        return;
    }

    Serial.println("HomeKit: Resetting pairing data...");
    homeSpan.deleteStoredValues();
    status = HOMEKIT_WAITING_FOR_PAIRING;
    Serial.println("HomeKit: Pairing reset complete - restart device to take effect");
}

void HomeKitController::printDiagnostics()
{
    Serial.println("HomeKit: ========== DIAGNOSTIC INFO ==========");
    Serial.printf("HomeKit: Initialized: %s\n", initialized ? "Yes" : "No");
    Serial.printf("HomeKit: Status: %s\n", getStatusString().c_str());
    Serial.printf("HomeKit: Setup Code: %s\n", setupCode.c_str());

    // Note: HomeSpan 1.9.1 doesn't provide getControllerCount()
    Serial.println("HomeKit: Pairing Status: Check serial output for pairing messages");

    Serial.printf("HomeKit: WiFi Status: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("HomeKit: WiFi IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("HomeKit: WiFi Hostname: %s\n", WiFi.getHostname());
        Serial.printf("HomeKit: mDNS Name: %s.local\n", WiFi.getHostname());
    }

    Serial.printf("HomeKit: Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("HomeKit: Uptime: %lu ms\n", millis());
    Serial.println("HomeKit: ======================================");
}

void HomeKitController::setPairingStatus(bool paired)
{
    if (paired)
    {
        status = HOMEKIT_PAIRED;
        Serial.println("HomeKit: Status manually set to PAIRED");
    }
    else
    {
        status = HOMEKIT_WAITING_FOR_PAIRING;
        Serial.println("HomeKit: Status manually set to WAITING_FOR_PAIRING");
    }
}

void HomeKitController::onPairingComplete(bool paired)
{
    if (paired)
    {
        status = HOMEKIT_RUNNING;
        Serial.println("HomeKit: PAIRING SUCCESSFUL! Device is now connected to HomeKit");
    }
    else
    {
        status = HOMEKIT_WAITING_FOR_PAIRING;
        Serial.println("HomeKit: Pairing removed or failed - back to waiting state");
    }
}
