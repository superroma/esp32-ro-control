#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ButtonLogic.h"
#include "WiFiController.h"
#include "HomeKitController.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define I2C_SDA 21
#define I2C_SCL 22

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Screen and Filter Management ---
#define BUTTON_LEFT_PIN 4  // Previous screen
#define BUTTON_RIGHT_PIN 5 // Next screen (changed from 2 to 5)
#define NUM_SCREENS 9      // Normal screens + WiFi status screen + HomeKit screen

enum ScreenType
{
  SCREEN_DASHBOARD,
  SCREEN_PP1,
  SCREEN_PP2,
  SCREEN_CARBON,
  SCREEN_MEMBRANE,
  SCREEN_MINERALIZER,
  SCREEN_USAGE,
  SCREEN_WIFI_STATUS,    // WiFi status screen
  SCREEN_HOMEKIT_STATUS, // New HomeKit status screen
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

// ButtonLogic instance for clean button handling
ButtonLogic buttonLogic;

// WiFi Controller instance
WiFiController wifiController;

// HomeKit Controller instance
HomeKitController homeKitController;

// Button hardware state (managed by interrupts)
volatile bool leftButtonCurrentlyPressed = false;
volatile bool rightButtonCurrentlyPressed = false;
volatile bool leftButtonJustReleased = false;
volatile bool rightButtonJustReleased = false;

// Filter data
FilterInfo filters[5] = {
    {"PP1 FILTER", "PP1", 80, STATUS_OK, "2 months"},
    {"PP2 FILTER", "PP2", 75, STATUS_OK, "2 months"},
    {"CARBON", "CAR", 50, STATUS_OK, "1 month"},
    {"MEMBRANE", "MEM", 60, STATUS_OK, "3 months"},
    {"MINERALIZR", "MIN", 15, STATUS_WARNING, "2 weeks"}};

unsigned int totalWaterUsed = 1234; // Liters

// Function declarations
void drawWiFiStatusScreen();
void drawHomeKitStatusScreen();
void drawCounterResetScreen();
void drawUsageScreen();
void drawFilterScreen(int filterIndex);
void drawDashboard();
void updateFilterStatus();

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
  // Detect press/release based on current pin state
  bool currentState = digitalRead(BUTTON_LEFT_PIN) == LOW; // LOW = pressed (with pullup)

  if (currentState && !leftButtonCurrentlyPressed)
  {
    // Button just pressed
    leftButtonCurrentlyPressed = true;
    Serial.println("Left button pressed!");
  }
  else if (!currentState && leftButtonCurrentlyPressed)
  {
    // Button just released
    leftButtonCurrentlyPressed = false;
    leftButtonJustReleased = true;
    Serial.println("Left button released!");
  }
}

void IRAM_ATTR handleRightButton()
{
  // Detect press/release based on current pin state
  bool currentState = digitalRead(BUTTON_RIGHT_PIN) == LOW; // LOW = pressed (with pullup)

  if (currentState && !rightButtonCurrentlyPressed)
  {
    // Button just pressed
    rightButtonCurrentlyPressed = true;
    Serial.println("Right button pressed!");
  }
  else if (!currentState && rightButtonCurrentlyPressed)
  {
    // Button just released
    rightButtonCurrentlyPressed = false;
    rightButtonJustReleased = true;
    Serial.println("Right button released!");
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("RO Monitor Starting...");

  Wire.begin(I2C_SDA, I2C_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(WHITE);

  // Setup both buttons
  pinMode(BUTTON_LEFT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_LEFT_PIN), handleLeftButton, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RIGHT_PIN), handleRightButton, CHANGE);

  Serial.println("Buttons configured:");
  Serial.print("Left button (GPIO ");
  Serial.print(BUTTON_LEFT_PIN);
  Serial.println(")");
  Serial.print("Right button (GPIO ");
  Serial.print(BUTTON_RIGHT_PIN);
  Serial.println(")");

  // Test button states
  Serial.print("Initial GPIO 4 state: ");
  Serial.println(digitalRead(BUTTON_LEFT_PIN));
  Serial.print("Initial GPIO 5 state: ");
  Serial.println(digitalRead(BUTTON_RIGHT_PIN));

  updateFilterStatus();
  lastScreenChange = millis();

  // Initialize WiFi - show setup screen first
  Serial.println("Initializing WiFi...");
  display.clearDisplay();

  // Show WiFi setup instructions
  display.setTextSize(1);
  drawCenteredText("WiFi Setup Required", 8, 1);

  // Network name
  display.setCursor(0, 22);
  display.print("Network:");
  display.setCursor(50, 22);
  display.print("RO-Monitor-Setup");

  // Password
  display.setCursor(0, 32);
  display.print("Password:");
  display.setCursor(50, 32);
  display.print("setup123");

  // IP Address
  display.setCursor(0, 42);
  display.print("Setup IP:");
  display.setCursor(50, 42);
  display.print("192.168.4.1");

  // Status
  drawCenteredText("Starting...", 54, 1);
  display.display();

  // Initialize WiFi controller
  wifiController.begin();

  // Show WiFi status after initialization
  delay(2000);
  drawWiFiStatusScreen();
  delay(3000);

  // Initialize HomeKit after WiFi (will start once WiFi is connected)
  Serial.println("Starting HomeKit initialization...");
  drawCenteredText("Starting HomeKit", 54, 1);
  display.display();
  delay(1000);
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

  const ResetState &resetState = buttonLogic.getResetState();

  if (resetState.showingResetProgress)
  {
    // Show progress bar while both buttons are held
    display.setTextSize(2);
    drawCenteredText("HOLD", 10, 2);
    drawCenteredText("BUTTONS", 30, 2);

    // Draw progress bar using the current progress value from ButtonLogic
    int barWidth = 100;
    int barHeight = 8;
    int barX = (SCREEN_WIDTH - barWidth) / 2;
    int barY = 50;

    // Progress bar background
    display.drawRect(barX, barY, barWidth, barHeight, WHITE);

    // Progress bar fill based on ButtonLogic progress
    int fillWidth = (barWidth - 2) * resetState.progressPercent / 100;
    if (fillWidth > 0)
    {
      display.fillRect(barX + 1, barY + 1, fillWidth, barHeight - 2, WHITE);
    }
  }
  else
  {
    // Show confirmation screen with context-specific messages
    if (currentScreen == SCREEN_WIFI_STATUS)
    {
      display.setTextSize(2);
      drawCenteredText("RESET", 0, 2);
      drawCenteredText("WIFI?", 20, 2);

      // Warning message
      display.setTextSize(1);
      drawCenteredText("This will reset", 42, 1);
      drawCenteredText("WiFi settings", 52, 1);
    }
    else
    {
      display.setTextSize(2);
      drawCenteredText("RESET", 0, 2);
      drawCenteredText("COUNTER?", 20, 2);

      // Warning message
      display.setTextSize(1);
      drawCenteredText("This will reset", 42, 1);
      drawCenteredText("all water usage", 52, 1);
    }

    // Physical button instructions at bottom
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print("CANCEL");

    display.setCursor(90, 56);
    display.print("OK");
  }

  display.display();
}

void drawWiFiStatusScreen()
{
  display.clearDisplay();

  // Title
  drawCenteredText("WIFI INFO", 0, 2);

  if (wifiController.isConnected())
  {
    // Network name (SSID)
    display.setTextSize(1);
    display.setCursor(0, 18);
    display.print("Network:");
    String ssid = wifiController.getSSID();
    if (ssid.length() > 10)
    {
      ssid = ssid.substring(0, 7) + "...";
    }
    display.setCursor(50, 18);
    display.print(ssid);

    // IP Address
    display.setCursor(0, 28);
    display.print("IP:");
    String ip = wifiController.getIPAddress();
    display.setCursor(20, 28);
    display.print(ip);

    // Signal strength
    int rssi = wifiController.getRSSI();
    display.setCursor(0, 38);
    display.print("Signal:");
    display.setCursor(45, 38);
    display.print(String(rssi) + "dBm");

    // Connection status
    display.setCursor(0, 48);
    display.print("Status: Connected");

    // Signal strength bar (visual indicator)
    int signalBars = 0;
    if (rssi > -50)
      signalBars = 4;
    else if (rssi > -60)
      signalBars = 3;
    else if (rssi > -70)
      signalBars = 2;
    else if (rssi > -80)
      signalBars = 1;

    // Draw signal bars in top right
    int barX = 100;
    int barY = 18;
    for (int i = 0; i < 4; i++)
    {
      int barHeight = (i + 1) * 2;
      if (i < signalBars)
      {
        display.fillRect(barX + i * 4, barY + 8 - barHeight, 3, barHeight, WHITE);
      }
      else
      {
        display.drawRect(barX + i * 4, barY + 8 - barHeight, 3, barHeight, WHITE);
      }
    }
  }
  else if (wifiController.getStatus() == WiFiStatus::CONFIG_MODE)
  {
    // Show setup instructions with network details
    display.setTextSize(1);
    drawCenteredText("WiFi Setup Mode", 18, 1);

    // Network name
    display.setCursor(0, 30);
    display.print("Network:");
    display.setCursor(50, 30);
    display.print(wifiController.getAPName());

    // Password
    display.setCursor(0, 40);
    display.print("Password:");
    display.setCursor(50, 40);
    display.print(wifiController.getAPPassword());

    // IP Address
    display.setCursor(0, 50);
    display.print("Setup IP:");
    display.setCursor(50, 50);
    display.print(wifiController.getAPIP());
  }
  else
  {
    // Show connection attempts or error
    drawCenteredText("Not Connected", 28, 1);
    drawCenteredText("Check Settings", 38, 1);
  }

  // Device name at bottom right
  display.setTextSize(1);
  String deviceName = wifiController.getDeviceName();
  if (deviceName.length() > 12)
  {
    deviceName = deviceName.substring(0, 9) + "...";
  }
  display.setCursor(128 - deviceName.length() * 6, 58);
  display.print(deviceName);

  display.display();
}

void drawHomeKitStatusScreen()
{
  display.clearDisplay();

  // Title
  drawCenteredText("HOMEKIT", 0, 2);

  HomeKitStatus hkStatus = homeKitController.getStatus();

  if (hkStatus == HOMEKIT_NOT_INITIALIZED)
  {
    // HomeKit not initialized yet
    display.setTextSize(1);
    drawCenteredText("Initializing...", 20, 1);
    drawCenteredText("Please wait", 30, 1);
  }
  else if (hkStatus == HOMEKIT_WAITING_FOR_PAIRING)
  {
    // Show pairing instructions
    display.setTextSize(1);
    drawCenteredText("Ready to Pair", 18, 1);

    // Setup code (large and prominent)
    display.setTextSize(2);
    String setupCode = homeKitController.getSetupCode();
    drawCenteredText(setupCode, 28, 2);

    // Instructions
    display.setTextSize(1);
    drawCenteredText("Enter in Home app", 48, 1);
    drawCenteredText("Add Accessory", 58, 1);
  }
  else if (hkStatus == HOMEKIT_PAIRED)
  {
    // Paired but not connected
    display.setTextSize(1);
    drawCenteredText("HomeKit Paired", 20, 1);
    drawCenteredText("Connecting...", 30, 1);

    // Show device count
    display.setCursor(0, 42);
    display.print("Devices: 6"); // 5 filters + 1 usage sensor

    // Show status
    display.setCursor(0, 52);
    display.print("Status: Paired");
  }
  else if (hkStatus == HOMEKIT_RUNNING)
  {
    // Connected and running
    display.setTextSize(1);
    drawCenteredText("HomeKit Active", 18, 1);

    // Connected icon
    display.fillCircle(10, 28, 3, WHITE);
    display.setCursor(18, 25);
    display.print("Connected");

    // Device count
    display.setCursor(0, 35);
    display.print("Filters: 5 active");

    // Usage sensor
    display.setCursor(0, 45);
    display.print("Usage: Monitoring");

    // Instructions
    display.setTextSize(1);
    display.setCursor(0, 55);
    display.print("Use iOS Home app");
  }
  else
  {
    // Error state
    display.setTextSize(1);
    drawCenteredText("HomeKit Error", 25, 1);
    drawCenteredText("Check connection", 38, 1);
  }

  display.display();
}

void processButtons()
{
  // Create button state from hardware interrupts
  ButtonState buttons;
  buttons.leftPressed = leftButtonCurrentlyPressed;
  buttons.rightPressed = rightButtonCurrentlyPressed;
  buttons.leftJustReleased = leftButtonJustReleased;
  buttons.rightJustReleased = rightButtonJustReleased;

  // Process buttons through the ButtonLogic class
  ButtonEvent event = buttonLogic.processButtons(buttons, millis());

  // Clear the release flags after processing
  leftButtonJustReleased = false;
  rightButtonJustReleased = false;

  // Handle the events from ButtonLogic
  switch (event)
  {
  case ButtonEvent::LEFT_RELEASED:
    // Previous screen (only normal screens)
    currentScreen = (currentScreen - 1 + NUM_SCREENS) % NUM_SCREENS;
    lastScreenChange = millis();
    Serial.println("Left button released - previous screen");
    break;

  case ButtonEvent::RIGHT_RELEASED:
    // Next screen (only normal screens)
    currentScreen = (currentScreen + 1) % NUM_SCREENS;
    lastScreenChange = millis();
    Serial.println("Right button released - next screen");
    break;

  case ButtonEvent::RESET_PROGRESS_STARTED:
    currentScreen = SCREEN_COUNTER_RESET;
    Serial.println("Reset progress started");
    break;

  case ButtonEvent::RESET_PROGRESS_UPDATED:
    // Progress is automatically tracked in ButtonLogic
    break;

  case ButtonEvent::RESET_CONFIRMATION_READY:
    Serial.println("Reset confirmation ready");
    break;

  case ButtonEvent::RESET_CANCELLED:
    Serial.println("Counter reset cancelled!");
    currentScreen = SCREEN_DASHBOARD;
    break;

  case ButtonEvent::RESET_CONFIRMED:
    Serial.println("Resetting counter!");
    // Reset counter
    totalWaterUsed = 0;
    // Reset all filter percentages to 100%
    for (int i = 0; i < 5; i++)
    {
      filters[i].percentage = 100;
      filters[i].status = STATUS_OK;
      filters[i].timeLeft = "12 months";
    }

    // Special case: if we're on WiFi screen, also reset WiFi settings
    if (currentScreen == SCREEN_WIFI_STATUS)
    {
      Serial.println("Also resetting WiFi settings!");
      wifiController.resetSettings();
      // Restart WiFi setup
      display.clearDisplay();
      drawCenteredText("WIFI RESET", 20, 2);
      drawCenteredText("Restarting...", 40, 1);
      display.display();
      delay(2000);
      wifiController.begin();
    }

    currentScreen = SCREEN_DASHBOARD;
    break;

  case ButtonEvent::NONE:
  default:
    // No action needed
    break;
  }
}

void loop()
{
  // Check for serial commands for testing (remove in production)
  if (Serial.available())
  {
    char cmd = Serial.read();
    switch (cmd)
    {
    case 'L':
    case 'l':
      // Simulate left button press and release
      Serial.println("SIMULATE: Left button press/release");
      leftButtonCurrentlyPressed = true;
      delay(50);
      leftButtonCurrentlyPressed = false;
      leftButtonJustReleased = true;
      break;
    case 'R':
    case 'r':
      // Simulate right button press and release
      Serial.println("SIMULATE: Right button press/release");
      rightButtonCurrentlyPressed = true;
      delay(50);
      rightButtonCurrentlyPressed = false;
      rightButtonJustReleased = true;
      break;
    case 'B':
    case 'b':
      // Simulate both buttons press
      Serial.println("SIMULATE: Both buttons pressed");
      leftButtonCurrentlyPressed = true;
      rightButtonCurrentlyPressed = true;
      break;
    case 'U':
    case 'u':
      // Simulate both buttons release
      Serial.println("SIMULATE: Both buttons released");
      leftButtonCurrentlyPressed = false;
      rightButtonCurrentlyPressed = false;
      leftButtonJustReleased = false;
      rightButtonJustReleased = false;
      break;
    case 'H':
    case 'h':
      Serial.println("HELP:");
      Serial.println("L/l = Left button press/release");
      Serial.println("R/r = Right button press/release");
      Serial.println("B/b = Both buttons press");
      Serial.println("U/u = Both buttons release");
      Serial.println("W/w = WiFi status");
      Serial.println("C/c = Start WiFi config portal");
      Serial.println("X/x = Reset WiFi settings");
      Serial.println("K/k = HomeKit status");
      Serial.println("D/d = HomeKit diagnostics");
      Serial.println("P/p = Reset HomeKit pairing");
      Serial.println("S/s = Set HomeKit as paired (for testing)");
      Serial.println("H/h = This help");
      break;
    case 'W':
    case 'w':
      Serial.println("WiFi Status:");
      Serial.printf("Status: %s\n", wifiController.getStatusString().c_str());
      Serial.printf("SSID: %s\n", wifiController.getSSID().c_str());
      Serial.printf("IP: %s\n", wifiController.getIPAddress().c_str());
      Serial.printf("RSSI: %d dBm\n", wifiController.getRSSI());
      Serial.printf("Device: %s\n", wifiController.getDeviceName().c_str());
      break;
    case 'C':
    case 'c':
      Serial.println("Starting WiFi config portal...");
      wifiController.startConfigPortal();
      break;
    case 'X':
    case 'x':
      Serial.println("Resetting WiFi settings...");
      wifiController.resetSettings();
      delay(1000);
      wifiController.begin();
      break;
    case 'K':
    case 'k':
      Serial.println("HomeKit Status:");
      Serial.printf("Status: %s\n", homeKitController.getStatusString().c_str());
      Serial.printf("Setup Code: %s\n", homeKitController.getSetupCode().c_str());
      Serial.printf("Paired: %s\n", homeKitController.isPaired() ? "Yes" : "No");
      break;
    case 'D':
    case 'd':
      homeKitController.printDiagnostics();
      break;
    case 'P':
    case 'p':
      Serial.println("Resetting HomeKit pairing...");
      homeKitController.resetPairing();
      break;
    case 'S':
    case 's':
      Serial.println("Setting HomeKit status to paired (for testing)...");
      homeKitController.setPairingStatus(true);
      break;
    }
  }

  // Process button inputs
  processButtons();

  // Update WiFi controller
  wifiController.update();

  // Initialize HomeKit when WiFi is connected (one-time initialization)
  static bool homeKitInitialized = false;
  if (!homeKitInitialized && wifiController.isConnected())
  {
    Serial.println("WiFi connected, initializing HomeKit...");
    homeKitController.begin(filters);
    homeKitInitialized = true;
  }

  // Update HomeKit controller if initialized
  if (homeKitInitialized)
  {
    homeKitController.update();
    homeKitController.updateFilterAccessories(filters);
  }

  // Update filter status
  updateFilterStatus();

  // Auto-rotate screens (only if not showing counter reset)
  if (!buttonLogic.isInResetMode() && millis() - lastScreenChange > screenInterval)
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
  case SCREEN_WIFI_STATUS:
    drawWiFiStatusScreen();
    break;
  case SCREEN_HOMEKIT_STATUS:
    drawHomeKitStatusScreen();
    break;
  case SCREEN_COUNTER_RESET:
    drawCounterResetScreen();
    break;
  }

  delay(100);
}
