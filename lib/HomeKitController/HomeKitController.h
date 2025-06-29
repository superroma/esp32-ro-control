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

class FilterMaintenanceAccessory : public Service::FilterMaintenance
{
private:
    SpanCharacteristic *filterChangeIndication;
    SpanCharacteristic *filterLifeLevel;
    SpanCharacteristic *resetFilterIndication;
    FilterInfo *filterRef;
    int filterIndex;

public:
    FilterMaintenanceAccessory(FilterInfo *filter, int index);
    boolean update() override;
    void updateFromFilter();
};

class HomeKitController
{
private:
    Preferences prefs;
    HomeKitStatus status;
    bool initialized;
    String setupCode;
    FilterMaintenanceAccessory *filterAccessories[5];
    unsigned long lastUpdate;
    const unsigned long updateInterval = 5000; // Update every 5 seconds

public:
    HomeKitController();
    void begin(FilterInfo filters[5]);
    void update();
    HomeKitStatus getStatus();
    String getSetupCode();
    bool isPaired();
    void updateFilterAccessories(FilterInfo filters[5]);
    String getStatusString();
    void resetPairing();
};

#endif
