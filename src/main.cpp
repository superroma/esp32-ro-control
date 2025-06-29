#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define I2C_SDA 21
#define I2C_SCL 22

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Screen and Filter Management ---
#define BUTTON_LEFT_PIN 4  // Previous screen
#define BUTTON_RIGHT_PIN 2 // Next screen
#define NUM_SCREENS 7      // Only normal screens (no counter reset)

enum ScreenType
{
  SCREEN_DASHBOARD,
  SCREEN_PP1,
  SCREEN_PP2,
  SCREEN_CARBON,
  SCREEN_MEMBRANE,
  SCREEN_MINERALIZER,
  SCREEN_USAGE,
  SCREEN_COUNTER_RESET
};

enum FilterStatus
{
  STATUS_OK,      // >20%
  STATUS_WARNING, // 10-20%
  STATUS_REPLACE  // <10%
};

struct FilterInfo
{
  String name;
  String shortName;
  int percentage;
  FilterStatus status;
  String timeLeft;
};

unsigned long lastScreenChange = 0;
const unsigned long screenInterval = 8000; // 8 seconds
volatile int currentScreen = 0;

// Button handling variables
volatile bool leftButtonPressed = false;
volatile bool rightButtonPressed = false;
volatile unsigned long leftButtonPressTime = 0;
volatile unsigned long rightButtonPressTime = 0;
volatile bool bothButtonsPressed = false;
volatile unsigned long bothButtonsStartTime = 0;
const unsigned long longPressTime = 3000; // 3 seconds

// Counter reset screen state
bool showingCounterReset = false;
bool resetConfirmSelected = false; // false = cancel, true = ok

// Filter data
FilterInfo filters[5] = {
    {"PP1 FILTER", "PP1", 80, STATUS_OK, "2 months"},
    {"PP2 FILTER", "PP2", 75, STATUS_OK, "2 months"},
    {"CARBON", "CAR", 50, STATUS_OK, "1 month"},
    {"MEMBRANE", "MEM", 60, STATUS_OK, "3 months"},
    {"MINERALIZR", "MIN", 15, STATUS_WARNING, "2 weeks"}};

unsigned int totalWaterUsed = 1234; // Liters

// Update filter status based on percentage
void updateFilterStatus()
{
  for (int i = 0; i < 5; i++)
  {
    if (filters[i].percentage < 10)
    {
      filters[i].status = STATUS_REPLACE;
    }
    else if (filters[i].percentage < 20)
    {
      filters[i].status = STATUS_WARNING;
    }
    else
    {
      filters[i].status = STATUS_OK;
    }
  }
}

// Graphics helper functions
void drawProgressBar(int x, int y, int width, int height, int percentage)
{
  // Draw border
  display.drawRect(x, y, width, height, WHITE);

  // Fill bar based on percentage
  int fillWidth = (width - 2) * percentage / 100;
  if (fillWidth > 0)
  {
    display.fillRect(x + 1, y + 1, fillWidth, height - 2, WHITE);
  }

  // Draw percentage text in center of bar
  display.setTextSize(2);
  String percentText = String(percentage) + "%";
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(percentText, 0, 0, &x1, &y1, &w, &h);
  int textX = x + (width - w) / 2;
  int textY = y + (height - h) / 2;

  // Draw text in inverse color for visibility
  if (fillWidth > w / 2)
  {
    // If bar is more than half full, draw black text on white background
    display.setTextColor(BLACK);
  }
  else
  {
    // If bar is less than half full, draw white text
    display.setTextColor(WHITE);
  }

  display.setCursor(textX, textY);
  display.print(percentText);
  display.setTextColor(WHITE); // Reset to white
}

void drawLargeStatusIcon(int x, int y, FilterStatus status)
{
  switch (status)
  {
  case STATUS_OK:
    // Large checkmark in circle
    display.drawCircle(x + 8, y + 8, 8, WHITE);
    display.drawLine(x + 4, y + 8, x + 7, y + 11, WHITE);
    display.drawLine(x + 7, y + 11, x + 12, y + 5, WHITE);
    // Make checkmark thicker
    display.drawLine(x + 4, y + 9, x + 7, y + 12, WHITE);
    display.drawLine(x + 7, y + 12, x + 12, y + 6, WHITE);
    break;
  case STATUS_WARNING:
    // Large exclamation mark in triangle
    display.drawTriangle(x + 8, y, x, y + 15, x + 16, y + 15, WHITE);
    // Exclamation mark
    display.drawLine(x + 8, y + 4, x + 8, y + 10, WHITE);
    display.drawLine(x + 7, y + 4, x + 7, y + 10, WHITE);
    display.drawLine(x + 9, y + 4, x + 9, y + 10, WHITE);
    display.fillRect(x + 7, y + 12, 3, 2, WHITE);
    break;
  case STATUS_REPLACE:
    // Large X in circle
    display.drawCircle(x + 8, y + 8, 8, WHITE);
    display.drawLine(x + 4, y + 4, x + 12, y + 12, WHITE);
    display.drawLine(x + 12, y + 4, x + 4, y + 12, WHITE);
    // Make X thicker
    display.drawLine(x + 5, y + 4, x + 13, y + 12, WHITE);
    display.drawLine(x + 13, y + 5, x + 5, y + 13, WHITE);
    break;
  }
}

void drawFilterCard(int x, int y, int width, int height, String name, FilterStatus status, int percentage)
{
  // Draw card border
  display.drawRect(x, y, width, height, WHITE);

  // Filter name at top
  display.setTextSize(1);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(name, 0, 0, &x1, &y1, &w, &h);
  int textX = x + (width - w) / 2;
  display.setCursor(textX, y + 2);
  display.print(name);

  // Status icon in center
  drawLargeStatusIcon(x + (width - 16) / 2, y + 12, status);

  // Percentage at bottom
  display.setTextSize(1);
  String percentText = String(percentage) + "%";
  display.getTextBounds(percentText, 0, 0, &x1, &y1, &w, &h);
  textX = x + (width - w) / 2;
  display.setCursor(textX, y + height - 10);
  display.print(percentText);
}

void drawCenteredText(String text, int y, int textSize)
{
  display.setTextSize(textSize);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, y);
  display.print(text);
}

void IRAM_ATTR handleLeftButton()
{
  leftButtonPressed = true;
  leftButtonPressTime = millis();

  if (rightButtonPressed && (millis() - rightButtonPressTime < 500))
  {
    bothButtonsPressed = true;
    bothButtonsStartTime = millis();
  }
}

void IRAM_ATTR handleRightButton()
{
  rightButtonPressed = true;
  rightButtonPressTime = millis();

  if (leftButtonPressed && (millis() - leftButtonPressTime < 500))
  {
    bothButtonsPressed = true;
    bothButtonsStartTime = millis();
  }
}

void setup()
{
  Wire.begin(I2C_SDA, I2C_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(WHITE);

  // Setup both buttons
  pinMode(BUTTON_LEFT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_LEFT_PIN), handleLeftButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RIGHT_PIN), handleRightButton, FALLING);

  updateFilterStatus();
  lastScreenChange = millis();
}

void drawDashboard()
{
  display.clearDisplay();

  // System title at top
  drawCenteredText("RO SYSTEM", 0, 2);

  // Draw filter cards in a grid layout
  int cardWidth = 40;
  int cardHeight = 30;
  int startX = 4;
  int startY = 18;
  int spacingX = 44;
  int spacingY = 32;

  // Row 1: PP1, PP2, Carbon
  drawFilterCard(startX, startY, cardWidth, cardHeight, "PP1", filters[0].status, filters[0].percentage);
  drawFilterCard(startX + spacingX, startY, cardWidth, cardHeight, "PP2", filters[1].status, filters[1].percentage);
  drawFilterCard(startX + spacingX * 2, startY, cardWidth, cardHeight, "CAR", filters[2].status, filters[2].percentage);

  // Row 2: Membrane, Mineralizer
  drawFilterCard(startX + spacingX / 2, startY + spacingY, cardWidth, cardHeight, "MEM", filters[3].status, filters[3].percentage);
  drawFilterCard(startX + spacingX / 2 + spacingX, startY + spacingY, cardWidth, cardHeight, "MIN", filters[4].status, filters[4].percentage);

  display.display();
}

void drawFilterScreen(int filterIndex)
{
  display.clearDisplay();

  FilterInfo filter = filters[filterIndex];

  // Filter name at top (large text)
  drawCenteredText(filter.name, 0, 2);

  // Large progress bar with percentage inside
  drawProgressBar(5, 25, 118, 20, filter.percentage);

  // Status text (large)
  display.setTextSize(2);
  String statusText;
  switch (filter.status)
  {
  case STATUS_OK:
    statusText = "OK";
    break;
  case STATUS_WARNING:
    statusText = "LOW";
    break;
  case STATUS_REPLACE:
    statusText = "REPLACE";
    break;
  }
  drawCenteredText(statusText, 50, 2);

  display.display();
}

void drawUsageScreen()
{
  display.clearDisplay();

  // Title (large)
  drawCenteredText("USAGE", 0, 2);

  // Large number display
  display.setTextSize(3);
  String waterText = String(totalWaterUsed);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(waterText, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, 20);
  display.print(waterText);

  // Units (large)
  display.setTextSize(2);
  display.getTextBounds("LITERS", 0, 0, &x1, &y1, &w, &h);
  x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, 45);
  display.print("LITERS");

  display.display();
}

void drawCounterResetScreen()
{
  display.clearDisplay();

  // Title
  drawCenteredText("RESET COUNTER", 0, 1);

  // Warning message
  display.setTextSize(1);
  drawCenteredText("This will reset", 15, 1);
  drawCenteredText("water usage to 0", 25, 1);

  // Options
  int buttonY = 45;
  int buttonWidth = 50;
  int buttonHeight = 15;
  int cancelX = 10;
  int okX = 68;

  // Cancel button
  if (!resetConfirmSelected)
  {
    display.fillRect(cancelX, buttonY, buttonWidth, buttonHeight, WHITE);
    display.setTextColor(BLACK);
  }
  else
  {
    display.drawRect(cancelX, buttonY, buttonWidth, buttonHeight, WHITE);
    display.setTextColor(WHITE);
  }
  display.setCursor(cancelX + 12, buttonY + 4);
  display.print("CANCEL");

  // OK button
  if (resetConfirmSelected)
  {
    display.fillRect(okX, buttonY, buttonWidth, buttonHeight, WHITE);
    display.setTextColor(BLACK);
  }
  else
  {
    display.drawRect(okX, buttonY, buttonWidth, buttonHeight, WHITE);
    display.setTextColor(WHITE);
  }
  display.setCursor(okX + 20, buttonY + 4);
  display.print("OK");

  display.setTextColor(WHITE); // Reset color
  display.display();
}

void processButtons()
{
  // Handle both buttons pressed for counter reset
  if (bothButtonsPressed)
  {
    if (millis() - bothButtonsStartTime >= longPressTime)
    {
      if (!showingCounterReset)
      {
        showingCounterReset = true;
        resetConfirmSelected = false; // Default to cancel
        currentScreen = SCREEN_COUNTER_RESET;
      }
      bothButtonsPressed = false;
      leftButtonPressed = false;
      rightButtonPressed = false;
    }
    return; // Don't process individual buttons while both are pressed
  }

  // Handle individual button presses
  if (leftButtonPressed)
  {
    leftButtonPressed = false;

    if (showingCounterReset)
    {
      resetConfirmSelected = !resetConfirmSelected; // Toggle selection
    }
    else
    {
      // Previous screen (only normal screens)
      currentScreen = (currentScreen - 1 + NUM_SCREENS) % NUM_SCREENS;
    }
    lastScreenChange = millis();
  }

  if (rightButtonPressed)
  {
    rightButtonPressed = false;

    if (showingCounterReset)
    {
      if (resetConfirmSelected)
      {
        // Reset counter
        totalWaterUsed = 0;
        // Reset all filter percentages to 100%
        for (int i = 0; i < 5; i++)
        {
          filters[i].percentage = 100;
          filters[i].status = STATUS_OK;
          filters[i].timeLeft = "12 months"; // Reset time estimates
        }
      }
      // Exit counter reset screen
      showingCounterReset = false;
      currentScreen = SCREEN_DASHBOARD;
    }
    else
    {
      // Next screen (only normal screens)
      currentScreen = (currentScreen + 1) % NUM_SCREENS;
    }
    lastScreenChange = millis();
  }

  // Reset both buttons pressed flag if buttons released
  if (!digitalRead(BUTTON_LEFT_PIN) && !digitalRead(BUTTON_RIGHT_PIN))
  {
    bothButtonsPressed = false;
  }
}

void loop()
{
  // Process button inputs
  processButtons();

  // Update filter status
  updateFilterStatus();

  // Auto-rotate screens (only if not showing counter reset)
  if (!showingCounterReset && millis() - lastScreenChange > screenInterval)
  {
    currentScreen = (currentScreen + 1) % NUM_SCREENS;
    lastScreenChange = millis();
  }

  // Draw current screen
  switch (currentScreen)
  {
  case SCREEN_DASHBOARD:
    drawDashboard();
    break;
  case SCREEN_PP1:
    drawFilterScreen(0); // PP1
    break;
  case SCREEN_PP2:
    drawFilterScreen(1); // PP2
    break;
  case SCREEN_CARBON:
    drawFilterScreen(2); // Carbon
    break;
  case SCREEN_MEMBRANE:
    drawFilterScreen(3); // Membrane
    break;
  case SCREEN_MINERALIZER:
    drawFilterScreen(4); // Mineralizer
    break;
  case SCREEN_USAGE:
    drawUsageScreen();
    break;
  case SCREEN_COUNTER_RESET:
    drawCounterResetScreen();
    break;
  }

  delay(100);
}
