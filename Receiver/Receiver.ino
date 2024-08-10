#include <esp_now.h>
#include <WiFi.h> 
#include <DHT.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <DHT.h>
#include <HardwareSerial.h>
#include <vector>

#define DHTTYPE DHT11

//change to the pins used on your setup
int DHTPin = 14;
int ledPin = 12;
int ledPin2 = 26;

DHT dht(DHTPin, DHTTYPE);

typedef struct data {
  float temp; 
  float hum;
} data;

data inboundData;

float hum;
float temp;

void dataReceived(const esp_now_recv_info *info, const uint8_t* incomingData, int len) {
  memcpy(&inboundData, incomingData, sizeof(inboundData));
  Serial.printf("Transmitter MAC Address: %02X:%02X:%02X:%02X:%02X:%02X \n\r", info-> src_addr[0],info-> src_addr[1],info-> src_addr[2],info-> src_addr[3],info-> src_addr[4],info-> src_addr[5]);
  Serial.println("Temp:");
  Serial.println(inboundData.temp);
  Serial.println("Hum:");
  Serial.println(inboundData.hum);
  Serial.println();

  temp = inboundData.temp;
  hum = inboundData.hum;
}

float getLastTemp() {
  return temp;
}
float getLastHum() {
  return hum;
}


Servo servo;

const char *ssid = //your SSID (WiFi Name);
const char *password = //your password;

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
            flex-direction: column;
            align-items: center;
            border-radius: 5px;
            padding-right: 10%;
            margin: 10px 10px;
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
    <p id="Temperature">Temperature:</p>
    <button class="button" onclick="getHumidity()">Get Humidity</button>
    <p id="Humidity">Humidity:</p>

    <!-- Start of addition -->
    <button class="button" onclick="getSentTemperature()">Get Other Temperature</button>
    <p id="SentTemperature">Sent Temperature:</p>
    <button class="button" onclick="getSentHumidity()">Get Other Humidity</button>
    <p id="SentHumidity">Sent Humidity:</p>
    <!-- End of addition -->
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
        fetch('/Temperature').then(response => response.text()).then(data => {
            document.getElementById('Temperature').innerText = `Temperature: ${data} °F`;
        });
      }

      function getHumidity() {
        fetch('/Humidity').then(response => response.text()).then(data => {
          document.getElementById('Humidity').innerText = `Humidity: ${data} %`;
        });
      }

        function getSentHumidity() {
          fetch('/sentHumidity').then(response => response.text()).then(data => {
            document.getElementById('SentHumidity').innerText = `Humidity: ${data} %`;
          });
        }

        function getSentTemperature() {
          fetch('/sentTemperature').then(response => response.text()).then(data => {
            document.getElementById('SentTemperature').innerText = `Temperature: ${data} °F`;
          });
        }
    </script>
</body>
<!-- END of Body -->
</html>
)rawliteral";

int pos = 0;

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


//DHT Handling
void handleRoomHumidity() {
  float roomHum = dht.readHumidity();
  if(isnan(roomHum)) {
    server.send(500, "text/plain", "No Data Availible");
  } else {
    server.send(200, "text/plain", String(roomHum));
  }
  flashLed();
}

void handleSentHumidity() {
  if(isnan(hum)) {
    server.send(500, "text/plain", "No Data Availible");
  } else {
    server.send(200, "text/plain", String(hum));
  }
  flashLed();
}

void handleRoomTemperature() {
  float roomTemp = dht.readTemperature(true);
  if(isnan(roomTemp)) {
    server.send(500, "text/plain", "No Data Availible");
  } else {
    server.send(200, "text/plain", String(roomTemp));
  }
  flashLed();
}

void handleSentTemperature() {
  if(isnan(temp)) {
    server.send(500, "text/plain", "No Data Availible");
  } else {
    server.send(200, "text/plain", String(temp));
  }
  flashLed();
}




void setup() {
  delay(3000);
  Serial.println("Starting Setup...");
  dht.begin();
  servo.attach(33);
  pinMode(ledPin,OUTPUT);
  pinMode(ledPin2,OUTPUT);
  digitalWrite(ledPin,LOW);
  digitalWrite(ledPin2,LOW);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Attempting WiFi Connection...");

  int checksum = 0;
  while(WiFi.status() != WL_CONNECTED && checksum <= 50) {
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
    server.on("/Temperature", handleRoomTemperature);
    server.on("/Humidity", handleRoomHumidity);
    server.on("/sentHumidity", handleSentHumidity);
    server.on("/sentTemperature", handleSentTemperature);
    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("HTTP Server has started");
    led200();


    if (esp_now_init() == ESP_OK) {
      Serial.println("ESP INIT SUCCESS");
    } else {
      Serial.println("ESP INIT FAILURE");
    }
  } else {
    Serial.println("");
    Serial.println("Failed to connect to WiFi");
  }
  esp_now_register_recv_cb(dataReceived);
  delay(500);
  Serial.println("Setup Complete. Waiting for input or broadcast...");
} 

void loop() {
  // Serial.println(WiFi.macAddress()); Use this to find the MAC of your micro-controller
  server.handleClient();
  delay(2000);
}
