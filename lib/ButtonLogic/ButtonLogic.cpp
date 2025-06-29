#include "ButtonLogic.h"
#include <algorithm>

ButtonLogic::ButtonLogic()
{
    reset();
}

void ButtonLogic::reset()
{
    bothButtonsWerePressed = false;
    waitingForBothRelease = false;
    ignoreNextReleases = false;
    resetProgressStartTime = 0;
    resetState = ResetState();
}

bool ButtonLogic::shouldProcessNormalButtons() const
{
    return !resetState.showingCounterReset && !waitingForBothRelease;
}

ButtonEvent ButtonLogic::processButtons(const ButtonState &buttons, unsigned long currentTimeMs)
{
    bool bothCurrentlyPressed = buttons.leftPressed && buttons.rightPressed;

    // === RESET LOGIC ===

    // 1. Both buttons just pressed - start reset progress
    if (bothCurrentlyPressed && !bothButtonsWerePressed)
    {
        bothButtonsWerePressed = true;
        resetProgressStartTime = currentTimeMs;
        resetState.showingResetProgress = true;
        resetState.showingCounterReset = true;
        resetState.resetConfirmationReady = false;
        resetState.progressPercent = 0;
        waitingForBothRelease = false;
        return ButtonEvent::RESET_PROGRESS_STARTED;
    }

    // 2. Both buttons still pressed - update progress
    else if (bothButtonsWerePressed && bothCurrentlyPressed && !resetState.resetConfirmationReady)
    {
        unsigned long elapsed = currentTimeMs - resetProgressStartTime;
        resetState.progressPercent = std::min(100, (int)((elapsed * 100) / longPressTime));

        if (elapsed >= longPressTime)
        {
            // Progress complete - show confirmation
            resetState.resetConfirmationReady = true;
            resetState.showingResetProgress = false;
            waitingForBothRelease = true; // Wait for both buttons to be released
            return ButtonEvent::RESET_CONFIRMATION_READY;
        }

        return ButtonEvent::RESET_PROGRESS_UPDATED;
    }

    // 3. Buttons released before progress complete - cancel
    else if (bothButtonsWerePressed && !bothCurrentlyPressed &&
             !waitingForBothRelease && !resetState.resetConfirmationReady)
    {
        bothButtonsWerePressed = false;
        resetState.showingResetProgress = false;
        resetState.showingCounterReset = false;
        resetState.resetConfirmationReady = false;
        resetState.progressPercent = 0;
        return ButtonEvent::RESET_CANCELLED;
    }

    // 4. Both buttons released after confirmation ready - enable individual button processing
    else if (waitingForBothRelease && !bothCurrentlyPressed)
    {
        waitingForBothRelease = false;
        bothButtonsWerePressed = false;
        ignoreNextReleases = true; // Ignore any releases that happened during this transition
        return ButtonEvent::NONE;
    }

    // === INDIVIDUAL BUTTON LOGIC ===

    // Clear ignore flag if no releases are pending
    if (ignoreNextReleases && !buttons.leftJustReleased && !buttons.rightJustReleased)
    {
        ignoreNextReleases = false;
    }

    // Process individual button releases only if not in reset mode
    if (shouldProcessNormalButtons())
    {
        if (buttons.leftJustReleased)
        {
            return ButtonEvent::LEFT_RELEASED;
        }
        if (buttons.rightJustReleased)
        {
            return ButtonEvent::RIGHT_RELEASED;
        }
    }

    // Process confirmation screen button releases (only if not ignoring)
    else if (resetState.showingCounterReset && resetState.resetConfirmationReady &&
             !waitingForBothRelease && !ignoreNextReleases)
    {
        if (buttons.leftJustReleased)
        {
            // Left button = Cancel
            resetState.showingCounterReset = false;
            resetState.showingResetProgress = false;
            resetState.resetConfirmationReady = false;
            resetState.progressPercent = 0;
            bothButtonsWerePressed = false;
            return ButtonEvent::RESET_CANCELLED;
        }

        if (buttons.rightJustReleased)
        {
            // Right button = OK - perform reset
            resetState.showingCounterReset = false;
            resetState.showingResetProgress = false;
            resetState.resetConfirmationReady = false;
            resetState.progressPercent = 0;
            bothButtonsWerePressed = false;
            return ButtonEvent::RESET_CONFIRMED;
        }
    }

    return ButtonEvent::NONE;
}
