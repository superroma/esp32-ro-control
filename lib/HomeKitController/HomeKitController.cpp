#include "HomeKitController.h"

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

FilterMaintenanceAccessory::FilterMaintenanceAccessory(FilterInfo *filter, int index) : Service::FilterMaintenance()
{
    filterRef = filter;
    filterIndex = index;

    // Initialize characteristics with proper values
    filterChangeIndication = new Characteristic::FilterChangeIndication(0);    // 0=No change needed, 1=Change needed
    filterLifeLevel = new Characteristic::FilterLifeLevel(filter->percentage); // Percentage remaining
    resetFilterIndication = new Characteristic::ResetFilterIndication();       // Write-only for reset

    // Set initial values based on current filter state
    updateFromFilter();

    Serial.printf("HomeKit: Created filter accessory %d (%s) with %d%% remaining\n",
                  index + 1, filter->name.c_str(), filter->percentage);
}

boolean FilterMaintenanceAccessory::update()
{
    // Handle reset filter indication
    if (resetFilterIndication->updated())
    {
        if (resetFilterIndication->getNewVal() == 1)
        {
            // Reset filter - set percentage to 100%
            filterRef->percentage = 100;
            filterRef->status = STATUS_OK;    // STATUS_OK
            filterRef->timeLeft = "6 months"; // Default time

            Serial.printf("HomeKit: Filter %d (%s) reset via HomeKit to 100%%\n",
                          filterIndex + 1, filterRef->name.c_str());

            // Update characteristics to reflect the reset
            filterLifeLevel->setVal(100);
            filterChangeIndication->setVal(0);

            // Return true to indicate successful processing
            return true;
        }
    }

    // Always return true to indicate the accessory is working properly
    return true;
}

void FilterMaintenanceAccessory::updateFromFilter()
{
    if (filterRef)
    {
        // Update filter life level (percentage)
        filterLifeLevel->setVal(filterRef->percentage);

        // Update change indication based on status
        int changeNeeded = (filterRef->status == STATUS_REPLACE) ? 1 : 0; // STATUS_REPLACE
        filterChangeIndication->setVal(changeNeeded);
    }
}

HomeKitController::HomeKitController()
{
    status = HOMEKIT_NOT_INITIALIZED;
    initialized = false;
    setupCode = "466-37-726"; // Default setup code
    lastUpdate = 0;

    // Initialize filter accessory pointers
    for (int i = 0; i < 5; i++)
    {
        filterAccessories[i] = nullptr;
    }
}

void HomeKitController::begin(FilterInfo filters[5])
{
    if (initialized)
    {
        Serial.println("HomeKit: Already initialized, skipping...");
        return;
    }

    Serial.println("HomeKit: ========== INITIALIZING HOMESPAN ==========");

    // Check WiFi status before starting
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("HomeKit: WARNING - WiFi not connected! HomeKit may not work properly.");
        Serial.printf("HomeKit: WiFi Status: %d\n", WiFi.status());
    }
    else
    {
        Serial.printf("HomeKit: WiFi connected - IP: %s, Hostname: %s\n",
                      WiFi.localIP().toString().c_str(), WiFi.getHostname());
    }

    // Initialize preferences for HomeKit data storage
    Serial.println("HomeKit: Initializing preferences...");
    prefs.begin("homekit", false);

    // Set up HomeSpan with improved configuration
    Serial.println("HomeKit: Starting HomeSpan.begin()...");
    try
    {
        homeSpan.begin(Category::Bridges, "RO Monitor Bridge");
        Serial.println("HomeKit: HomeSpan.begin() completed successfully");
    }
    catch (...)
    {
        Serial.println("HomeKit: ERROR - HomeSpan.begin() failed!");
        status = HOMEKIT_ERROR;
        return;
    }

    // Enable maximum logging for debugging
    Serial.println("HomeKit: Setting log level to maximum (2)...");
    homeSpan.setLogLevel(2);

    // Use the default setup code
    setupCode = "466-37-726"; // Default HomeSpan setup code
    Serial.printf("HomeKit: Using setup code: %s\n", setupCode.c_str());

    // Create bridge accessory (required for multiple accessories)
    Serial.println("HomeKit: Creating bridge accessory...");
    new SpanAccessory();
    new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Manufacturer("DIY Electronics");
    new Characteristic::SerialNumber("RO001");
    new Characteristic::Model("ESP32-RO-v1");
    new Characteristic::FirmwareRevision("1.0.0");
    Serial.println("HomeKit: Bridge accessory created");

    // Create filter accessories
    const char *filterNames[] = {"PP1 Filter", "PP2 Filter", "Carbon Filter", "RO Membrane", "Mineralizer"};

    Serial.println("HomeKit: Creating filter accessories...");
    for (int i = 0; i < 5; i++)
    {
        Serial.printf("HomeKit: Creating filter accessory %d: %s\n", i + 1, filterNames[i]);

        // Create a new accessory for each filter
        new SpanAccessory();
        new Service::AccessoryInformation();
        new Characteristic::Identify();
        new Characteristic::Manufacturer("DIY Electronics");
        new Characteristic::SerialNumber(("FILTER" + String(i + 1)).c_str());
        new Characteristic::Model("RO Filter");
        new Characteristic::Name(filterNames[i]);
        new Characteristic::FirmwareRevision("1.0.0");

        // Add filter maintenance service
        filterAccessories[i] = new FilterMaintenanceAccessory(&filters[i], i);
        Serial.printf("HomeKit: Filter accessory %d created successfully\n", i + 1);

        // Small delay to ensure proper initialization
        delay(10);
    }

    Serial.println("HomeKit: Creating water usage sensor...");
    // Create water usage sensor accessory
    new SpanAccessory();
    new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Manufacturer("DIY Electronics");
    new Characteristic::SerialNumber("USAGE001");
    new Characteristic::Model("Water Usage Sensor");
    new Characteristic::Name("Water Usage");
    new Characteristic::FirmwareRevision("1.0.0");

    new Service::LeakSensor();           // Use leak sensor as a placeholder for water monitoring
    new Characteristic::LeakDetected(0); // 0 = No leak detected
    new Characteristic::StatusActive(1); // 1 = Active
    new Characteristic::StatusFault(0);  // 0 = No fault
    Serial.println("HomeKit: Water usage sensor created");

    Serial.println("HomeKit: All accessories created successfully!");

    // Final initialization
    initialized = true;
    status = HOMEKIT_WAITING_FOR_PAIRING;

    Serial.println("HomeKit: ========== HOMEKIT READY ==========");
    Serial.printf("HomeKit: Setup code: %s\n", setupCode.c_str());
    Serial.printf("HomeKit: Device name: RO Monitor Bridge\n");
    Serial.printf("HomeKit: Total accessories: 7 (1 bridge + 5 filters + 1 sensor)\n");
    Serial.println("HomeKit: Ready for pairing with iOS Home app!");
    Serial.println("HomeKit: Look for 'RO Monitor Bridge' in Home app");
    Serial.println("HomeKit: ==========================================");
}

void HomeKitController::update()
{
    if (!initialized)
    {
        return;
    }

    // Update HomeSpan - this is critical and should be called frequently
    homeSpan.poll();

    // Simple status monitoring without unavailable methods
    static unsigned long lastStatusCheck = 0;
    static unsigned long lastConnectionLog = 0;

    // Check general status every 5 seconds
    if (millis() - lastStatusCheck > 5000)
    {
        // In HomeSpan 1.9.1, we don't have direct access to controller count
        // We'll rely on the setup code being displayed and manual status updates
        Serial.println("HomeKit: Status check - HomeSpan running, waiting for pairing...");

        // Assume we're waiting for pairing unless manually updated
        if (status == HOMEKIT_NOT_INITIALIZED)
        {
            status = HOMEKIT_WAITING_FOR_PAIRING;
        }

        lastStatusCheck = millis();
    }

    // Log connection status every 15 seconds
    if (millis() - lastConnectionLog > 15000)
    {
        Serial.printf("HomeKit: WiFi: %s, Setup Code: %s\n",
                      WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected",
                      setupCode.c_str());

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.printf("HomeKit: WiFi IP: %s, mDNS advertising as '%s.local'\n",
                          WiFi.localIP().toString().c_str(),
                          WiFi.getHostname());
        }

        lastConnectionLog = millis();
    }

    // Periodically update filter accessories
    if (millis() - lastUpdate > updateInterval)
    {
        for (int i = 0; i < 5; i++)
        {
            if (filterAccessories[i])
            {
                filterAccessories[i]->updateFromFilter();
            }
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

void HomeKitController::updateFilterAccessories(FilterInfo filters[5])
{
    if (!initialized)
    {
        return;
    }

    for (int i = 0; i < 5; i++)
    {
        if (filterAccessories[i])
        {
            filterAccessories[i]->updateFromFilter();
        }
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
