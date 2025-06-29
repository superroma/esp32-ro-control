#include <unity.h>
#include "ButtonLogic.h"

ButtonLogic *buttonLogic;

void setUp(void)
{
    buttonLogic = new ButtonLogic();
    buttonLogic->setLongPressTime(1000); // 1 second for faster testing
}

void tearDown(void)
{
    delete buttonLogic;
}

// Test normal button releases
void test_individual_button_releases()
{
    ButtonState buttons;

    // Test left button release
    buttons.leftJustReleased = true;
    ButtonEvent event = buttonLogic->processButtons(buttons, 0);
    TEST_ASSERT_EQUAL(ButtonEvent::LEFT_RELEASED, event);

    // Reset state
    buttons.leftJustReleased = false;

    // Test right button release
    buttons.rightJustReleased = true;
    event = buttonLogic->processButtons(buttons, 0);
    TEST_ASSERT_EQUAL(ButtonEvent::RIGHT_RELEASED, event);
}

// Test reset progress flow
void test_reset_progress_complete_flow()
{
    ButtonState buttons;
    unsigned long time = 0;

    // 1. Press both buttons - should start reset progress
    buttons.leftPressed = true;
    buttons.rightPressed = true;
    ButtonEvent event = buttonLogic->processButtons(buttons, time);
    TEST_ASSERT_EQUAL(ButtonEvent::RESET_PROGRESS_STARTED, event);
    TEST_ASSERT_TRUE(buttonLogic->getResetState().showingResetProgress);
    TEST_ASSERT_EQUAL(0, buttonLogic->getResetState().progressPercent);

    // 2. Keep holding - progress should update
    time = 500; // 50% through
    event = buttonLogic->processButtons(buttons, time);
    TEST_ASSERT_EQUAL(ButtonEvent::RESET_PROGRESS_UPDATED, event);
    TEST_ASSERT_EQUAL(50, buttonLogic->getResetState().progressPercent);

    // 3. Complete the hold - should show confirmation
    time = 1000; // 100% complete
    event = buttonLogic->processButtons(buttons, time);
    TEST_ASSERT_EQUAL(ButtonEvent::RESET_CONFIRMATION_READY, event);
    TEST_ASSERT_TRUE(buttonLogic->getResetState().resetConfirmationReady);
    TEST_ASSERT_FALSE(buttonLogic->getResetState().showingResetProgress);

    // 4. Release both buttons - should enable individual processing
    buttons.leftPressed = false;
    buttons.rightPressed = false;
    event = buttonLogic->processButtons(buttons, time);
    TEST_ASSERT_EQUAL(ButtonEvent::NONE, event); // No event, just state change

    // 5. Clear release flags and make a deliberate left button press (cancel)
    buttons.leftJustReleased = false;
    buttons.rightJustReleased = false;
    event = buttonLogic->processButtons(buttons, time); // Clear the ignore flag
    
    buttons.leftJustReleased = true;
    event = buttonLogic->processButtons(buttons, time);
    TEST_ASSERT_EQUAL(ButtonEvent::RESET_CANCELLED, event);
    TEST_ASSERT_FALSE(buttonLogic->getResetState().showingCounterReset);
}

// Test reset cancelled by early release
void test_reset_cancelled_early_release()
{
    ButtonState buttons;
    unsigned long time = 0;

    // 1. Press both buttons
    buttons.leftPressed = true;
    buttons.rightPressed = true;
    ButtonEvent event = buttonLogic->processButtons(buttons, time);
    TEST_ASSERT_EQUAL(ButtonEvent::RESET_PROGRESS_STARTED, event);

    // 2. Release buttons before completion
    time = 500; // Only 50% through
    buttons.leftPressed = false;
    buttons.rightPressed = false;
    event = buttonLogic->processButtons(buttons, time);
    TEST_ASSERT_EQUAL(ButtonEvent::RESET_CANCELLED, event);
    TEST_ASSERT_FALSE(buttonLogic->getResetState().showingCounterReset);
}

// Test reset confirmed
void test_reset_confirmed()
{
    ButtonState buttons;
    unsigned long time = 0;

    // Complete the reset flow up to confirmation
    buttons.leftPressed = true;
    buttons.rightPressed = true;
    buttonLogic->processButtons(buttons, time); // Start

    time = 1000;
    buttonLogic->processButtons(buttons, time); // Complete

    buttons.leftPressed = false;
    buttons.rightPressed = false;
    buttonLogic->processButtons(buttons, time); // Release both

    // Clear release flags and make a deliberate right button press (confirm)
    buttons.leftJustReleased = false;
    buttons.rightJustReleased = false;
    buttonLogic->processButtons(buttons, time); // Clear the ignore flag
    
    buttons.rightJustReleased = true;
    ButtonEvent event = buttonLogic->processButtons(buttons, time);
    TEST_ASSERT_EQUAL(ButtonEvent::RESET_CONFIRMED, event);
    TEST_ASSERT_FALSE(buttonLogic->getResetState().showingCounterReset);
}

// Test that normal buttons are ignored during reset
void test_normal_buttons_ignored_during_reset()
{
    ButtonState buttons;

    // Start reset
    buttons.leftPressed = true;
    buttons.rightPressed = true;
    buttonLogic->processButtons(buttons, 0);

    // Try to press individual buttons - should be ignored
    buttons.leftJustReleased = true;
    ButtonEvent event = buttonLogic->processButtons(buttons, 500);
    TEST_ASSERT_NOT_EQUAL(ButtonEvent::LEFT_RELEASED, event);

    buttons.leftJustReleased = false;
    buttons.rightJustReleased = true;
    event = buttonLogic->processButtons(buttons, 500);
    TEST_ASSERT_NOT_EQUAL(ButtonEvent::RIGHT_RELEASED, event);
}

// Test that releasing both buttons after confirmation doesn't immediately trigger actions
void test_releasing_both_buttons_after_confirmation_requires_deliberate_press()
{
    ButtonState buttons;
    unsigned long time = 0;

    // 1. Complete the reset flow up to confirmation
    buttons.leftPressed = true;
    buttons.rightPressed = true;
    buttonLogic->processButtons(buttons, time); // Start progress

    time = 1000; // Complete progress
    ButtonEvent event = buttonLogic->processButtons(buttons, time);
    TEST_ASSERT_EQUAL(ButtonEvent::RESET_CONFIRMATION_READY, event);
    TEST_ASSERT_TRUE(buttonLogic->getResetState().resetConfirmationReady);

    // 2. Release both buttons - this should NOT trigger any action
    // Simulate the real scenario: when user releases both buttons,
    // the interrupt handlers set the release flags
    buttons.leftPressed = false;
    buttons.rightPressed = false;
    buttons.leftJustReleased = true;  // This would be set by interrupt
    buttons.rightJustReleased = true; // This would be set by interrupt

    // Processing this should NOT trigger CANCEL or CONFIRM
    event = buttonLogic->processButtons(buttons, time);
    TEST_ASSERT_EQUAL(ButtonEvent::NONE, event);

    // Confirmation screen should still be showing
    TEST_ASSERT_TRUE(buttonLogic->getResetState().showingCounterReset);
    TEST_ASSERT_TRUE(buttonLogic->getResetState().resetConfirmationReady);

    // 3. Clear the release flags (as main loop would do) and clear ignore flag
    buttons.leftJustReleased = false;
    buttons.rightJustReleased = false;
    buttonLogic->processButtons(buttons, time); // This clears the ignore flag

    // 4. Now make a deliberate left button press (cancel)
    buttons.leftJustReleased = true;
    event = buttonLogic->processButtons(buttons, time);
    TEST_ASSERT_EQUAL(ButtonEvent::RESET_CANCELLED, event);
    TEST_ASSERT_FALSE(buttonLogic->getResetState().showingCounterReset);
}

// Test the exact hardware scenario - both buttons released with flags set
void test_hardware_scenario_both_buttons_released_with_flags()
{
    ButtonState buttons;
    unsigned long time = 0;

    // Complete reset flow to confirmation
    buttons.leftPressed = true;
    buttons.rightPressed = true;
    buttonLogic->processButtons(buttons, time);
    time = 1000;
    ButtonEvent event = buttonLogic->processButtons(buttons, time);
    TEST_ASSERT_EQUAL(ButtonEvent::RESET_CONFIRMATION_READY, event);

    // Hardware scenario: Both buttons are released simultaneously
    // The interrupts fire and set both release flags at the same time
    buttons.leftPressed = false;
    buttons.rightPressed = false;
    buttons.leftJustReleased = true;
    buttons.rightJustReleased = true;

    // First call - this should transition out of waitingForBothRelease
    event = buttonLogic->processButtons(buttons, time);
    TEST_ASSERT_EQUAL(ButtonEvent::NONE, event); // Should ignore the releases

    // BUT IMPORTANT: In hardware, the main loop might not clear the release flags immediately
    // If we call processButtons again with the same flags still set...
    // This is where the bug happens!
    event = buttonLogic->processButtons(buttons, time);

    // The issue: currently this might return RESET_CANCELLED because
    // waitingForBothRelease is now false, and it processes leftJustReleased
    TEST_ASSERT_NOT_EQUAL(ButtonEvent::RESET_CANCELLED, event);
    TEST_ASSERT_NOT_EQUAL(ButtonEvent::RESET_CONFIRMED, event);
    TEST_ASSERT_EQUAL(ButtonEvent::NONE, event);

    // Screen should still show confirmation
    TEST_ASSERT_TRUE(buttonLogic->getResetState().showingCounterReset);
    TEST_ASSERT_TRUE(buttonLogic->getResetState().resetConfirmationReady);
}

void setup()
{
    UNITY_BEGIN();

    RUN_TEST(test_individual_button_releases);
    RUN_TEST(test_reset_progress_complete_flow);
    RUN_TEST(test_reset_cancelled_early_release);
    RUN_TEST(test_reset_confirmed);
    RUN_TEST(test_normal_buttons_ignored_during_reset);
    RUN_TEST(test_releasing_both_buttons_after_confirmation_requires_deliberate_press);
    RUN_TEST(test_hardware_scenario_both_buttons_released_with_flags);

    UNITY_END();
}

void loop()
{
    // Empty
}

int main()
{
    setup();
    return 0;
}
