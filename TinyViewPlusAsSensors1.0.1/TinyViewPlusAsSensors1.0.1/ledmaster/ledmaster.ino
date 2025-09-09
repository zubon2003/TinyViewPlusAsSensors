

#include <FastLED.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// LED settings
#define DATA_PIN 10 // Change to the data pin you are using for the master's LEDs
#define NUM_LEDS 300 // Change to the number of LEDs for the master
#define BLINK_INTERVAL 150
#define BLINK_COUNT 3

// ESP-NOW settings
#define CHANNEL 14
uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Command definitions
#define CAM0_COMMAND "L FF0000" //RED
#define CAM1_COMMAND "L 00FF00" //GREEN
#define CAM2_COMMAND "L 0000FF" //BLUE
#define CAM3_COMMAND "L FFFF00" //YELLOW

#define DEBUG true

// Global LED variables
CRGB leds[NUM_LEDS];
bool rainbowMode = true;
unsigned long lastRainbowUpdate = 0;
uint8_t rainbowDelay = 20; // Default rainbow speed

// Forward declarations
void startRainbow();
void stopRainbow();
void rainbowCycle();
void blinkColor(CRGB color, int count, int intervalMs);
CRGB hexToCRGB(const String& hex);
void executeCommand(const String& command);

// ESP-NOW callback (not used in master)
void onReceiveData(const esp_now_recv_info_t *recvInfo, const uint8_t *incomingData, int len) {
  // Master does not need to receive data in this setup.
}

// Function to send a command via ESP-NOW
void sendCommand(const String& command) {
  esp_err_t sendResult = esp_now_send(broadcastAddr, (const uint8_t *)command.c_str(), command.length());
#if DEBUG
  Serial.print("Sending command: '");
  Serial.print(command);
  Serial.print("'... ");
  if (sendResult == ESP_OK) {
    Serial.println("Success");
  } else {
    Serial.println("Failed");
  }
#endif
}

void setup() {
  Serial.begin(115200);
  delay(2000); // Wait for serial to initialize

  // LED initialization
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(64);
  FastLED.clear();
  FastLED.show();
  Serial.println("LED Master Controller Starting...");

  // WiFi setup
  WiFi.mode(WIFI_STA);
  esp_wifi_set_ps(WIFI_PS_NONE);
  WiFi.disconnect();
  delay(1000);

#if DEBUG
  Serial.print("This ESP32 MAC address: ");
  Serial.println(WiFi.macAddress());
#endif

  // Set a fixed channel
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);

  // Initialize ESP-NOW
  if (esp_now_init() == ESP_OK) {
#if DEBUG
    Serial.println("ESP-NOW Ready");
#endif
    esp_now_register_recv_cb(onReceiveData);
  } else {
#if DEBUG
    Serial.println("ESP-NOW init failed");
#endif
    return;
  }

  // Add broadcast peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddr, 6);
  peerInfo.channel = CHANNEL;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
#if DEBUG
    Serial.println("Failed to add broadcast peer");
#endif
  }

  startRainbow();

#if DEBUG
  Serial.println("\nMaster mode ready - enter a single character command:");
  Serial.println("  S - Start: Rainbow speed to max (S 0)");
  Serial.println("  E - End: Rainbow speed to default (S 20)");
  Serial.println("  0 - Blink Red 3 times");
  Serial.println("  1 - Blink Green 3 times");
  Serial.println("  2 - Blink Blue 3 times");
  Serial.println("  3 - Blink Yellow 3 times");
#endif
}

void loop() {
  // Handle serial commands
  if (Serial.available()) {
    char inChar = (char)Serial.read();
    String command = "";

    switch (inChar) {
      case 'S':
        command = "S 0"; // Fast Rainbow
        break;
      case 'E':
        command = "S 20"; // Slow Rainbow
        break;
      case '0':
        command = CAM0_COMMAND;
        break;
      case '1':
        command = CAM1_COMMAND;
        break;
      case '2':
        command = CAM2_COMMAND;
        break;
      case '3':
        command = CAM3_COMMAND;
        break;
    }

    if (command != "") {
      // Execute command locally on master
      executeCommand(command);
      // Send command to slaves
      sendCommand(command);
    }
  }

  // Update LED effects
  if (rainbowMode) {
    rainbowCycle();
  }
  delay(1); // Reduce CPU load
}

// --- LED Control Functions ---

void startRainbow() {
  rainbowMode = true;
  lastRainbowUpdate = 0;
#if DEBUG
  Serial.println("Rainbow mode started");
#endif
}

void stopRainbow() {
  rainbowMode = false;
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
#if DEBUG
  Serial.println("Rainbow mode stopped");
#endif
}

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

void blinkColor(CRGB color, int count, int intervalMs) {
  stopRainbow();
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
  if (cleanHex.length() != 6) return CRGB::Black;
  long number = strtol(cleanHex.c_str(), NULL, 16);
  return CRGB((number >> 16) & 0xFF, (number >> 8) & 0xFF, number & 0xFF);
}

void executeCommand(const String& command) {
#if DEBUG
  Serial.printf("Executing command locally: %s\n", command.c_str());
#endif
  if (command.startsWith("L ")) {
    String colorStr = command.substring(2);
    colorStr.trim();
    CRGB color = hexToCRGB(colorStr);
    blinkColor(color, BLINK_COUNT, BLINK_INTERVAL);
  } else if (command.startsWith("S ")) {
    int delay = command.substring(2).toInt();
    if (delay >= 0 && delay <= 1000) {
      rainbowDelay = delay;
    }
  }
}