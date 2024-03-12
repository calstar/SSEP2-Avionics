#include "WiFi.h"

const char* ssid     = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

void setup(){
  Serial.begin(115200);
  
  Serial.println();
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  delay(100);
}

void loop(){
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
  delay(2000);
}