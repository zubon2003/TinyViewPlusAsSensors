//Yatutose LED CONTROL SYSTEM(YLED) MIT LISENCE

#include <FastLED.h>
#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h"

#define NUM_LEDS 300
#define DATA_PIN 9
#define MAX_COMMAND_LENGTH 50
#define BLINK_INTERVAL 150
#define BLINK_COUNT 3

CRGB leds[NUM_LEDS];
bool rainbowMode = true;
unsigned long rainbowStartTime = 0;
unsigned long lastRainbowUpdate = 0;

// Rainbow speed setting
uint8_t rainbowDelay = 20; // Default delay

// Error handling enum
enum ErrorCode {
  ERROR_NONE = 0,
  ERROR_ESPNOW_INIT,
  ERROR_INVALID_COMMAND,
  ERROR_MEMORY
};

// Forward declarations
void onReceiveData(const esp_now_recv_info_t *recvInfo, const uint8_t *incomingData, int len);
void startRainbow();
void handleError(ErrorCode error);
void rainbowCycle();
ErrorCode processCommand(const String& inputString);


void setup() {
  // LED initialization
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(64);
  FastLED.clear();
  FastLED.show();
  Serial.begin(115200);
  delay(2000); // Wait for serial to initialize
  Serial.println("LED Slave Controller Starting...");

  // WiFi setup
  Serial.println("Initializing WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);

  // Print MAC address for debugging
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Set ESP-NOW channel (must match master)
  if (esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    Serial.println("Error setting WiFi channel");
  } else {
    Serial.println("WiFi channel set to 1");
  }

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    handleError(ERROR_ESPNOW_INIT);
    return;
  }
  Serial.println("ESP-NOW initialized successfully");

  // Register receive callback
  if (esp_now_register_recv_cb(onReceiveData) != ESP_OK) {
    Serial.println("Error registering receive callback");
  } else {
    Serial.println("Receive callback registered");
  }

  startRainbow();
  Serial.println("Slave mode ready - waiting for commands");
}

void loop() {
  if (rainbowMode) {
    rainbowCycle();
  }
  // Reduce CPU load
  delay(1);
}

void startRainbow() {
  rainbowMode = true;
  rainbowStartTime = millis();
  lastRainbowUpdate = 0;
  Serial.println("Rainbow mode started");
}

void stopRainbow() {
  rainbowMode = false;
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  Serial.println("Rainbow mode stopped");
}

// Non-blocking rainbow cycle
void rainbowCycle() {
  unsigned long currentTime = millis();
  if (currentTime - lastRainbowUpdate >= rainbowDelay) {
    static uint8_t hue = 0;
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(hue + (i * 10), 255, 255);
    }
    FastLED.show();
    hue += 5;
    lastRainbowUpdate = currentTime;
  }
}

// Non-blocking color display
void setColor(CRGB color, int durationMs) {
  static unsigned long colorStartTime = 0;
  static bool colorModeActive = false;
  static CRGB currentColor;
  static int colorDuration;

  if (!colorModeActive) {
    stopRainbow();
    fill_solid(leds, NUM_LEDS, color);
    FastLED.show();
    colorStartTime = millis();
    colorModeActive = true;
    currentColor = color;
    colorDuration = durationMs;
    Serial.printf("Color set: R=%d G=%d B=%d for %dms\n", color.r, color.g, color.b, durationMs);
  }

  if (millis() - colorStartTime >= colorDuration) {
    colorModeActive = false;
    startRainbow();
  }
}

void blinkColor(CRGB color, int count, int intervalMs) {
  stopRainbow();
  Serial.printf("Blinking color: R=%d G=%d B=%d, count=%d\n", color.r, color.g, color.b, count);
  for (int i = 0; i < count; i++) {
    fill_solid(leds, NUM_LEDS, color);
    FastLED.show();
    delay(intervalMs);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    if (i < count - 1) delay(intervalMs);
  }
  startRainbow();
}

CRGB hexToCRGB(const String& hex) {
  String cleanHex = hex;
  if (cleanHex.startsWith("#")) {
    cleanHex = cleanHex.substring(1);
  }
  if (cleanHex.length() != 6) {
    Serial.printf("Invalid hex color format: %s\n", hex.c_str());
    return CRGB::White;
  }
  for (size_t i = 0; i < cleanHex.length(); i++) {
    char c = cleanHex.charAt(i);
    if (!isxdigit(c)) {
      Serial.printf("Invalid hex character in color: %c\n", c);
      return CRGB::White;
    }
  }
  long number = strtol(cleanHex.c_str(), NULL, 16);
  return CRGB((number >> 16) & 0xFF, (number >> 8) & 0xFF, number & 0xFF);
}

ErrorCode processCommand(const String& inputString) {
  if (inputString.length() == 0) {
    return ERROR_INVALID_COMMAND;
  }
  Serial.printf("Processing command: %s\n", inputString.c_str());

  if (inputString.startsWith("F ")) {
    if (inputString.length() < 4) return ERROR_INVALID_COMMAND;
    char code = inputString.charAt(2);
    int duration = inputString.substring(4).toInt();
    if (duration <= 0 || duration > 10000) {
      Serial.println("Invalid duration");
      return ERROR_INVALID_COMMAND;
    }
    switch (code) {
      case 'R': setColor(CRGB::Red, duration); break;
      case 'G': setColor(CRGB::Green, duration); break;
      case 'Y': setColor(CRGB::Yellow, duration); break;
      case 'W': setColor(CRGB::White, duration); break;
      case 'B': setColor(CRGB::Blue, duration); break;
      case 'P': setColor(CRGB::Purple, duration); break;
      default:
        Serial.printf("Unknown color code: %c\n", code);
        return ERROR_INVALID_COMMAND;
    }
  } else if (inputString.startsWith("L ")) {
    if (inputString.length() < 4) return ERROR_INVALID_COMMAND;
    String colorStr = inputString.substring(2);
    colorStr.trim();
    if (colorStr == "OFF") {
      stopRainbow();
    } else if (colorStr == "ON" || colorStr == "RAINBOW") {
      startRainbow();
    } else {
      CRGB color = hexToCRGB(colorStr);
      blinkColor(color, BLINK_COUNT, BLINK_INTERVAL);
    }
  } else if (inputString.startsWith("B ")) {
    int brightness = inputString.substring(2).toInt();
    if (brightness >= 0 && brightness <= 255) {
      FastLED.setBrightness(brightness);
      FastLED.show();
      Serial.printf("Brightness set to: %d\n", brightness);
    }
    else {
      Serial.println("Invalid brightness value (0-255)");
      return ERROR_INVALID_COMMAND;
    }
  } else if (inputString.startsWith("S ")) {
    int delay = inputString.substring(2).toInt();
    if (delay >= 0 && delay <= 1000) {
      rainbowDelay = delay;
      Serial.printf("Rainbow speed set to: %dms\n", delay);
    } else {
      Serial.println("Invalid speed value (0-1000)");
      return ERROR_INVALID_COMMAND;
    }
  } else {
    Serial.printf("Unknown command: %s\n", inputString.c_str());
    return ERROR_INVALID_COMMAND;
  }
  return ERROR_NONE;
}

// Receive callback
void onReceiveData(const esp_now_recv_info_t *recvInfo, const uint8_t *incomingData, int len) {
  if (len > 0 && len <= MAX_COMMAND_LENGTH) {
    String cmd = "";
    for (int i = 0; i < len; i++) {
      cmd += (char)incomingData[i];
    }
    Serial.printf("Command received: '%s'\n", cmd.c_str());
    ErrorCode result = processCommand(cmd);
    if (result != ERROR_NONE) {
      Serial.printf("Command execution failed with error: %d\n", result);
    }
  } else {
    Serial.printf("Invalid data length received: %d\n", len);
  }
}

void handleError(ErrorCode error) {
  switch (error) {
    case ERROR_ESPNOW_INIT:
      Serial.println("Critical: ESP-NOW initialization failed");
      for (int i = 0; i < 5; i++) {
        fill_solid(leds, NUM_LEDS, CRGB::Red);
        FastLED.show();
        delay(200);
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        FastLED.show();
        delay(200);
      }
      break;
    case ERROR_INVALID_COMMAND:
      Serial.println("Invalid command received");
      break;
    case ERROR_MEMORY:
      Serial.println("Memory allocation error");
      break;
    default:
      Serial.printf("Unknown error: %d\n", error);
  }
}

// Debug function
void printSystemInfo() {
  Serial.println("=== System Information ===");
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("WiFi channel: %d\n", WiFi.channel());
  Serial.printf("LED count: %d\n", NUM_LEDS);
  Serial.printf("Brightness: %d\n", FastLED.getBrightness());
  Serial.println("===========================");
}
