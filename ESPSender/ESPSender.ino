//Lucas Saavedra Vaz's skeleton as in server
#include <DHT.h>
#include "ESP32_NOW.h"
#include "WiFi.h"
#include <esp_mac.h>  // For the MAC2STR and MACSTR macros

/* Definitions */

#define ESPNOW_WIFI_CHANNEL 6
#define DHTTYPE DHT11

int DHTPin = 23;

DHT dht(DHTPin, DHTTYPE);

typedef struct struct_message {
  float Temperature = 0;
  float Humidity = 0;
} struct_message;

struct_message myData;

/* Classes */

// Creating a new class that inherits from the ESP_NOW_Peer class is required.

class ESP_NOW_Broadcast_Peer : public ESP_NOW_Peer {
public:
  // Constructor of the class using the broadcast address
  ESP_NOW_Broadcast_Peer(uint8_t channel, wifi_interface_t iface, const uint8_t *lmk) : ESP_NOW_Peer(ESP_NOW.BROADCAST_ADDR, channel, iface, lmk) {}

  // Destructor of the class
  ~ESP_NOW_Broadcast_Peer() {
    remove();
  }

  // Function to properly initialize the ESP-NOW and register the broadcast peer
  bool begin() {
    if (!ESP_NOW.begin() || !add()) {
      log_e("Failed to initialize ESP-NOW or register the broadcast peer");
      return false;
    }
    return true;
  }

  // Function to send a message to all devices within the network
  bool send_message(const uint8_t *data, size_t len) {
    if (!send(data, len)) {
      log_e("Failed to broadcast message");
      return false;
    }
    return true;
  }
};

/* Global Variables */

uint32_t msg_count = 0;

// Create a broadcast peer object
ESP_NOW_Broadcast_Peer broadcast_peer(ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, NULL);

/* Main */

void setup() {
  dht.begin();
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  // Initialize the Wi-Fi module
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) {
    delay(100);
  }

  Serial.println("ESP-NOW - Broadcast Master (Sender)");
  Serial.println("Wi-Fi parameters:");
  Serial.println("  Mode: STA");
  Serial.println("  MAC Address: " + WiFi.macAddress());
  Serial.printf("  Channel: %d\n", ESPNOW_WIFI_CHANNEL);

  // Register the broadcast peer
  if (!broadcast_peer.begin()) {
    Serial.println("Failed to initialize broadcast peer");
    Serial.println("Reebooting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }

  Serial.println("Setup complete. Broadcasting messages every 5 seconds.");
}

void loop() {

    myData.Temperature = dht.readTemperature(true);
    myData.Humidity = dht.readHumidity();

    if (isnan(myData.Temperature) || isnan(myData.Humidity)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(ESP_NOW.BROADCAST_ADDR, (uint8_t *) &myData, sizeof(myData));
    
    if (result == ESP_OK) {
        Serial.println("Sent with success");
        Serial.println("Temp");
        Serial.println(myData.Temperature);
        Serial.println("Humidity");
        Serial.println(myData.Humidity);
    } else {
        Serial.println("Error sending the data");
        Serial.println("Temp");
        Serial.println(myData.Temperature);
        Serial.println("Humidity");
        Serial.println(myData.Humidity);
    }

  delay(5000);
}
