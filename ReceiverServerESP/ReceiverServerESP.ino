/*
    ESP-NOW Broadcast Slave
    Lucas Saavedra Vaz - 2024

    This sketch demonstrates how to receive broadcast messages from a master device using the ESP-NOW protocol.

    The master device will broadcast a message every 5 seconds to all devices within the network.

    The slave devices will receive the broadcasted messages. If they are not from a known master, they will be registered as a new master
    using a callback function.
*/

//Using Lucas Saavedra Vaz's script as the skeleton of the connections, most comments are by him

#include <ESPmDNS.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <DHT.h>
#include <HardwareSerial.h>
#include <esp_now.h>
#include "ESP32_NOW.h"
#include "WiFi.h"

#include <esp_mac.h>  // For the MAC2STR and MACSTR macros

#include <vector>

/* Definitions */

#define ESPNOW_WIFI_CHANNEL 6
#define DHTTYPE DHT11

int DHTPin = 23;

DHT dht(DHTPin, DHTTYPE);

/* Data Structure for Incoming Data */
typedef struct struct_message {
  float temperature;
  float humidity;
} struct_message;

/* Classes */

// Creating a new class that inherits from the ESP_NOW_Peer class is required.

class ESP_NOW_Peer_Class : public ESP_NOW_Peer {
public:
  // Constructor of the class
  ESP_NOW_Peer_Class(const uint8_t *mac_addr, uint8_t channel, wifi_interface_t iface, const uint8_t *lmk)
    : ESP_NOW_Peer(mac_addr, channel, iface, lmk) {}

  // Destructor of the class
  ~ESP_NOW_Peer_Class() {}

  // Function to register the master peer
  bool add_peer() {
    if (!add()) {
      log_e("Failed to register the broadcast peer");
      return false;
    }
    return true;
  }


  //todo
  // Static function to handle received data
  // static void onDataRecvStatic(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  //   if (instance) {
  //     instance->onReceive(mac_addr, incomingData, len);
  //   }
  // }

  // Function to print the received messages from the master
  void onReceive(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
    struct_message data;
    memcpy(&data, incomingData, sizeof(data));

    Serial.print("Temperature: ");
    Serial.println(data.temperature);
    Serial.print("Humidity: ");
    Serial.println(data.humidity);
    Serial.println();

    // Update class members with received data
    lastTemperature = data.temperature;
    lastHumidity = data.humidity;
  }

  float getLastTemperature() {
    return lastTemperature;
  }

  float getLastHumidity() {
    return lastHumidity;
  }

  // Static pointer to hold instance pointer
  static ESP_NOW_Peer_Class* instance;

private:
  float lastTemperature = 0.0;
  float lastHumidity = 0.0;
};

ESP_NOW_Peer_Class* ESP_NOW_Peer_Class::instance = nullptr;

/* Global Variables */

Servo servo;  // create servo object to control a servo
int ledPin = 12;
int ledPin2 = 26;

const char *ssid = //Your Wifi Name;
const char *password = //Your Wifi Password;

WebServer server(80);

const char *htmlContent = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Document</title>
    <style>
        html {
            font-family: 'Courier New', Courier, monospace;
            margin: 0;
            padding: 0;
            background-color: rgba(0, 0, 0, 0.8);
            height: 100%;
            width: 100%;
        }

        body {
            color: #fff;
            display: flex;
            flex-direction: column;
            align-items: center;
        }

        .button {
            border-radius: 10px;
            padding: 10px 15px;
            margin: 20px 20px;
        }

        .weatherSection {
            background-color: black;
            display: flex;
            flex-direction: row;
            align-items: center;
            border-radius: 5px;
            padding-right: 10%;
            margin: 10px 10px;
        }

        #temperature {
            padding-right: 20%;
        }
    </style>
</head>
<!-- START of Body -->
<body>
    <h1>Server PC Switch + Ambient Temp Monitor!</h1>
    <p>Press the button to turn PC On/Off</p>
    <button class="button" onclick="moveServoOpen()">Cycle Left</button>
    <button class="button" onclick="moveServoClose()">Cycle Right</button>
    <footer>Both buttons work to cycle both ways I just like to confuse myself</footer>
    <h2>Current Rooms Temperature Readings...</h2>
    <div class="weatherSection">
    <button class="button" onclick="getTemperature()">Get Temperature</button>
    <p id="temperature">Temperature:</p>
    <button class="button" onclick="getHumidity()">Get Humidity</button>
    <p id="humidity">Humidity:</p>
    </div>
    <script>
       message = "Servo has been moved"
      function moveServoOpen() {
        fetch('/moveServoOpen').then(response => response.text()).then(data => {
          console.log(data);
          alert(message + " left!");
        });
      }

      function moveServoClose() {
        fetch('/moveServoClose').then(response => response.text()).then(data => {
          console.log(data);
          alert(message + " right!");
        });
      }

      function getTemperature() {
        fetch('/temperature').then(response => response.text()).then(data => {
            document.getElementById('temperature').innerText = `Temperature: ${data} °F`;
        });
      }

      function getHumidity() {
        fetch('/humidity').then(response => response.text()).then(data => {
          document.getElementById('humidity').innerText = `Humidity: ${data} %`;
        });
      }
    </script>
</body>
<!-- END of Body -->
</html>
)rawliteral";

// For Servo Position
int pos = 0;

// List of all the masters. It will be populated when a new master is registered
std::vector<ESP_NOW_Peer_Class> masters;

/* Callbacks */

// // Callback called when an unknown peer sends a message
void register_new_master(const esp_now_recv_info_t *info, const uint8_t *data, int len, void *arg) {
  if (memcmp(info->des_addr, ESP_NOW.BROADCAST_ADDR, 6) == 0) {
    Serial.printf("Unknown peer " MACSTR " sent a broadcast message\n", MAC2STR(info->src_addr));
    Serial.println("Registering the peer as a master");

    ESP_NOW_Peer_Class new_master(info->src_addr, ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, NULL);
    masters.push_back(new_master);
    if (!masters.back().add_peer()) {
      Serial.println("Failed to register the new master");
      return;
    }

    // Register the receive callback for this master
    // ESP_NOW_Peer_Class::instance = &masters.back();
    // esp_now_register_recv_cb(ESP_NOW_Peer_Class::onDataRecvStatic);
  } else {
    // The slave will only receive broadcast messages
    log_v("Received a unicast message from " MACSTR, MAC2STR(info->src_addr));
    log_v("Ignoring the message");
  }
}

void flashLed() {
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin2, HIGH);
  delay(1000);
  digitalWrite(ledPin, LOW);
  delay(100);
  digitalWrite(ledPin2, LOW);
  delay(100);
}

void errLed() {
  digitalWrite(ledPin2, HIGH);
  delay(500);
  digitalWrite(ledPin2, LOW);
  delay(100);
}

void led200() {
  digitalWrite(ledPin, HIGH);
  delay(400);
  digitalWrite(ledPin, LOW);
  delay(100);
}

void handleRoot() {
  server.send(200, "text/html", htmlContent);
  led200();
}

void handleMoveServoOpen() {
  pos = servo.read();
  Serial.println(pos);
  if (pos <= 0) {
    servo.write(180);
    delay(15);
    server.send(200, "text/plain", "Servo moved");
    flashLed();
  } else {
    servo.write(0);
    delay(15);
    server.send(200, "text/plain", "Servo moved");
    flashLed();
  }
}

void handleMoveServoClose() {
  pos = servo.read();
  Serial.println(pos);
  if (pos >= 160) {
    servo.write(0);
    delay(15);
    server.send(200, "text/plain", "Servo moved");
    flashLed();
  } else {
    servo.write(180);
    delay(15);
    server.send(200, "text/plain", "Servo moved");
    flashLed();
  }
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  errLed();
}

void handleTemperature() {
  // Send the last received temperature data
  float roomTemp = dht.readTemperature();
  float temp = ESP_NOW_Peer_Class::instance ? ESP_NOW_Peer_Class::instance->getLastTemperature() : NAN;
  //ESP_NOW_Peer_Class::instance ? ESP_NOW_Peer_Class::instance->getLastTemperature() : NAN;
  if (isnan(temp) || isnan(roomTemp)) {
    server.send(500, "text/plain", "No data available");
  } else {
    server.send(200, "text/plain", String(temp));
  }
  flashLed();
}

void handleHumidity() {
  // Send the last received humidity data
  float roomHum = dht.readHumidity();
  float hum = ESP_NOW_Peer_Class::instance ? ESP_NOW_Peer_Class::instance->getLastHumidity() : NAN;
  //ESP_NOW_Peer_Class::instance ? ESP_NOW_Peer_Class::instance->getLastHumidity() : NAN;
  if (isnan(hum) || isnan(roomHum)) {
    server.send(500, "text/plain", "No data available");
  } else {
    server.send(200, "text/plain", String(hum));
  }
  flashLed();
}

/* Main */

void setup() {

  delay(5000);
  Serial.println("Starting setup...");
  dht.begin();
  servo.attach(33);
  pinMode(ledPin, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  digitalWrite(ledPin, LOW);
  digitalWrite(ledPin2, LOW);
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Attempting WiFi Connection...");

  int checksum = 0;
  while (WiFi.status() != WL_CONNECTED && checksum <= 50) {
    delay(500);
    Serial.print("...attempt # ");
    Serial.println(checksum);
    checksum++;
    errLed();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("esp32")) {
      Serial.println("MDNS responder started");
      led200();
    }

    server.on("/", handleRoot);
    server.on("/moveServoOpen", handleMoveServoOpen);
    server.on("/moveServoClose", handleMoveServoClose);
    server.on("/temperature", handleTemperature);
    server.on("/humidity", handleHumidity);
    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("HTTP server started");
    led200();
  } else {
    Serial.println("");
    Serial.println("Failed to connect to WiFi");
  }
  //end http server
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

  Serial.println("ESP-NOW - Broadcast Reciever and Server");
  Serial.println("Wi-Fi parameters:");
  Serial.println("  Mode: STA");
  Serial.println("  MAC Address: " + WiFi.macAddress());
  Serial.printf("  Channel: %d\n", ESPNOW_WIFI_CHANNEL);

  // Initialize the ESP-NOW protocol
  if (!ESP_NOW.begin()) {
    Serial.println("Failed to initialize ESP-NOW");
    Serial.println("Rebooting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }

  // Register the new peer callback
  ESP_NOW.onNewPeer(register_new_master, NULL);
  delay(500);

  Serial.println("Setup complete. Waiting for a master to broadcast a message...");
}

void loop() {
  server.handleClient();
  delay(1000);
}
