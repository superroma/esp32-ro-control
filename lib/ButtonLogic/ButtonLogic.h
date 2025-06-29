#pragma once

enum class ButtonEvent
{
    NONE,
    LEFT_RELEASED,
    RIGHT_RELEASED,
    RESET_PROGRESS_STARTED,
    RESET_PROGRESS_UPDATED,
    RESET_CONFIRMATION_READY,
    RESET_CANCELLED,
    RESET_CONFIRMED
};

struct ButtonState
{
    bool leftPressed = false;
    bool rightPressed = false;
    bool leftJustReleased = false;
    bool rightJustReleased = false;
};

struct ResetState
{
    bool showingCounterReset = false;
    bool showingResetProgress = false;
    bool resetConfirmationReady = false;
    int progressPercent = 0;
};

class ButtonLogic
{
private:
    // State tracking
    bool bothButtonsWerePressed = false;
    bool waitingForBothRelease = false;
    bool ignoreNextReleases = false; // Flag to ignore release events after both buttons released
    unsigned long resetProgressStartTime = 0;
    const unsigned long longPressTime = 3000; // 3 seconds

    // Internal state
    ResetState resetState;

public:
    ButtonLogic();

    // Main processing function - call this every loop
    ButtonEvent processButtons(const ButtonState &currentButtons, unsigned long currentTimeMs);

    // State accessors
    const ResetState &getResetState() const { return resetState; }
    bool isInResetMode() const { return resetState.showingCounterReset; }
    bool shouldProcessNormalButtons() const;

    // For testing
    void reset();
    void setLongPressTime(unsigned long timeMs) { const_cast<unsigned long &>(longPressTime) = timeMs; }
};
