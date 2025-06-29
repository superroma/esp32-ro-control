#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>

// OLED display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // Most common I2C address for SSD1306

// I2C pins for ESP32 DevKit
#define SDA_PIN 21
#define SCL_PIN 22

// Create display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Preferences object for storing data in NVS (Non-Volatile Storage)
Preferences preferences;

// Counter variable
unsigned long counter = 0;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 1000; // 1 second

// Function declarations
void updateDisplay();

void setup()
{
    Serial.begin(115200);

    // Initialize I2C with custom pins
    Wire.begin(SDA_PIN, SCL_PIN);

    // Initialize the display
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }

    // Clear the buffer
    display.clearDisplay();
    display.display();

    // Initialize preferences
    preferences.begin("counter", false); // "counter" is the namespace, false = read/write mode

    // Read the stored counter value (default to 0 if not found)
    counter = preferences.getULong("value", 0);

    Serial.print("Starting counter from: ");
    Serial.println(counter);

    // Display startup message
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("ESP32 Counter"));
    display.println(F("Starting..."));
    display.print(F("From: "));
    display.println(counter);
    display.display();

    delay(2000); // Show startup message for 2 seconds
}

void loop()
{
    unsigned long currentTime = millis();

    // Update counter every second
    if (currentTime - lastUpdate >= updateInterval)
    {
        counter++;
        lastUpdate = currentTime;

        // Store the new counter value in NVS
        preferences.putULong("value", counter);

        // Update display
        updateDisplay();

        // Print to serial for debugging
        Serial.print("Counter: ");
        Serial.println(counter);
    }

    // Small delay to prevent excessive CPU usage
    delay(10);
}

void updateDisplay()
{
    display.clearDisplay();

    // Title
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("ESP32 Counter"));
    display.println();

    // Counter value (large text)
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.print(F("Count: "));
    display.println(counter);

    // Additional info
    display.setTextSize(1);
    display.setCursor(0, 45);
    display.print(F("Uptime: "));
    display.print(millis() / 1000);
    display.println(F("s"));

    display.setCursor(0, 55);
    display.println(F("Persistent Storage"));

    display.display();
}
