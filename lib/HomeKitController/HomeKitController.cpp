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

    // Initialize characteristics
    filterChangeIndication = new Characteristic::FilterChangeIndication(0);    // 0=No change needed, 1=Change needed
    filterLifeLevel = new Characteristic::FilterLifeLevel(filter->percentage); // Percentage remaining
    resetFilterIndication = new Characteristic::ResetFilterIndication();       // Write-only for reset

    // Set initial values
    updateFromFilter();
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

            Serial.printf("HomeKit: Filter %d reset via HomeKit\n", filterIndex + 1);

            // Update characteristics to reflect the reset
            filterLifeLevel->setVal(100);
            filterChangeIndication->setVal(0);
        }
    }

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
        return;
    }

    Serial.println("HomeKit: Initializing HomeSpan...");

    // Initialize preferences for HomeKit data storage
    prefs.begin("homekit", false);

    // Set up HomeSpan with default setup code
    homeSpan.begin(Category::Bridges, "RO Monitor Bridge");

    // Enable web log for debugging
    homeSpan.setLogLevel(1);

    // Use the default setup code
    setupCode = "466-37-726"; // Default HomeSpan setup code

    // Create bridge accessory (required for multiple accessories)
    new SpanAccessory();
    new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Manufacturer("DIY Electronics");
    new Characteristic::SerialNumber("RO001");
    new Characteristic::Model("ESP32-RO-v1");
    new Characteristic::FirmwareRevision("1.0.0");

    // Create filter accessories
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

        // Add filter maintenance service
        filterAccessories[i] = new FilterMaintenanceAccessory(&filters[i], i);
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

    new Service::LeakSensor();           // Use leak sensor as a placeholder for water monitoring
    new Characteristic::LeakDetected(0); // 0 = No leak detected
    new Characteristic::StatusActive(1); // 1 = Active
    new Characteristic::StatusFault(0);  // 0 = No fault

    Serial.println("HomeKit: Accessories created, starting HomeSpan...");

    initialized = true;
    status = HOMEKIT_WAITING_FOR_PAIRING;

    Serial.printf("HomeKit: Setup code is %s\n", setupCode.c_str());
    Serial.println("HomeKit: Ready for pairing!");
}

void HomeKitController::update()
{
    if (!initialized)
    {
        return;
    }

    // Update HomeSpan
    homeSpan.poll();

    // Update status - simplified for now
    status = HOMEKIT_RUNNING;

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
    // Simplified - assume paired if initialized
    return true;
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
        return;
    }

    Serial.println("HomeKit: Resetting pairing data...");
    homeSpan.deleteStoredValues();
    status = HOMEKIT_WAITING_FOR_PAIRING;
    Serial.println("HomeKit: Pairing reset complete");
}
