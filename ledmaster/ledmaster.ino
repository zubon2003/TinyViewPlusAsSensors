#define DEBUG false

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// The broadcast address for ESP-NOW
uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Callback function for when data is received (not used in master)
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

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(1000); // For MAC address and WiFi stabilization

#if DEBUG
  Serial.print("This ESP32 MAC address: ");
  Serial.println(WiFi.macAddress());
#endif

  // Set a fixed channel (must match on master and slave)
  int channel = 1;
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

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
  peerInfo.channel = channel;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
#if DEBUG
    Serial.println("Failed to add broadcast peer");
#endif
  }

#if DEBUG
  Serial.println("\nMaster mode ready - enter a single character command:");
  Serial.println("  S - Start: Rainbow speed to max (S 0)");
  Serial.println("  E - End: Rainbow speed to default (S 20)");
  Serial.println("  0 - Blink Red 3 times");
  Serial.println("  1 - Blink Green 3 times");
  Serial.println("  2 - Blink Blue 3 times");
#endif
}

void loop() {
  // Check if there is data available to read from the serial port
  if (Serial.available()) {
    // Read the incoming character
    char inChar = (char)Serial.read();

    // Map the single character to the corresponding command and send it immediately
    switch (inChar) {
      case 'S':
        sendCommand("S 0");//Fast Rainbow
        break;
      case 'E':
        sendCommand("S 20");//Slow Rainbow
        break;
      case '0':
        sendCommand("L FF0000"); // Red
        break;
      case '1':
        sendCommand("L 00FF00"); // Green
        break;
      case '2':
        sendCommand("L 0000FF"); // Blue
        break;
      // default: ignore any other characters
    }
  }
}
