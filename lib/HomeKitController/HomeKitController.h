#ifndef HOMEKIT_CONTROLLER_H
#define HOMEKIT_CONTROLLER_H

#include "HomeSpan.h"
#include <Preferences.h>

// Forward declaration
struct FilterInfo;

enum HomeKitStatus
{
    HOMEKIT_NOT_INITIALIZED,
    HOMEKIT_WAITING_FOR_PAIRING,
    HOMEKIT_PAIRED,
    HOMEKIT_RUNNING,
    HOMEKIT_ERROR
};

// Custom pairing callback to track HomeKit pairing status
extern void homeKitPairingCallback(bool isPaired);

// Filter maintenance service using proper HomeKit FilterMaintenance service
struct DEV_FilterMaintenance : Service::FilterMaintenance
{
    SpanCharacteristic *filterChangeIndication; // 0=NO_CHANGE_NEEDED, 1=CHANGE_NEEDED
    SpanCharacteristic *filterLifeLevel;        // Filter life remaining as percentage (0-100)
    SpanCharacteristic *resetFilterIndication;  // Write-only characteristic for filter reset
    FilterInfo *filterRef;
    int filterIndex;

    DEV_FilterMaintenance(FilterInfo *filter, int index);
    void loop() override;
    boolean update() override;
    void updateFromFilter();
};

// Water usage sensor that reports total water usage
struct DEV_WaterUsageSensor : Service::TemperatureSensor
{
    SpanCharacteristic *temperature; // We'll use temperature to represent water usage (in hundreds of liters)
    unsigned int *waterUsageRef;

    DEV_WaterUsageSensor(unsigned int *waterUsage);
    void loop() override;
    void updateFromUsage();
};

class HomeKitController
{
private:
    Preferences prefs;
    HomeKitStatus status;
    bool initialized;
    String setupCode;
    DEV_FilterMaintenance *filterMaintenanceServices[5];
    DEV_WaterUsageSensor *waterUsageSensor;
    unsigned long lastUpdate;
    const unsigned long updateInterval = 10000; // Update every 10 seconds

public:
    HomeKitController();
    void begin(FilterInfo filters[5], unsigned int *waterUsage);
    void update();
    HomeKitStatus getStatus();
    String getSetupCode();
    bool isPaired();
    void updateSensors(FilterInfo filters[5], unsigned int waterUsage);
    String getStatusString();
    void resetPairing();
    void printDiagnostics();             // New diagnostic method
    void setPairingStatus(bool paired);  // Manual pairing status update
    void onPairingComplete(bool paired); // Callback for pairing status
};

#endif
