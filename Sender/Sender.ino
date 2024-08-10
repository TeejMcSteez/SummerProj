#include <esp_now.h>
#include <WiFi.h> 
#include <DHT.h>


#define DHTTYPE DHT11
#define DHTPin 13

DHT dht(DHTPin, DHTTYPE);

//change to your receivers MAC address
uint8_t receiverAddress[] = {0x00,0x00,0x00,0x00,0x00,0x00};
esp_now_peer_info_t peerInfo;

typedef struct data {
  float temp;
  float hum;
} data;

data sentData;

void dataSent(const uint8_t *macAddr, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Success"); 
  } else {
    Serial.println("err, or sent out of clock");
  }
}

void setup() {
  Serial.begin(115200); 
  delay(1000);
  WiFi.mode(WIFI_STA);
  dht.begin();

  if (esp_now_init() == ESP_OK) {
    Serial.println("ESP NOW INIT SUCCESSFUL");
    Serial.println(WiFi.macAddress());
  } else {
    Serial.println("ESP INIT FAILED");
    return;
  }

  esp_now_register_send_cb(dataSent); 

  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("failed to add peer!");
    return;
  }
}

void loop() {
  float temp = dht.readTemperature(true);
  float hum = dht.readHumidity();
  sentData.temp = temp;
  sentData.hum = hum;
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *) &sentData, sizeof(sentData));
  if (result != ESP_OK) {
    Serial.println("sending error");
  }
  Serial.println("Real Readings");
  Serial.println(temp);
  Serial.println(hum);
  delay(3000);
}