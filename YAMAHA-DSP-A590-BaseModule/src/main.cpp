#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <IRremote.hpp>

const char* ssid = "VOR-Verstärker";
const char* pass = "***REMOVED***";

void setupWifi(){
  delay(100);
  Serial.println("\nConnecting to");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED){
    delay(100);
    Serial.print("-");
  }

  Serial.println("\nConected to");
  Serial.println(ssid);
}

void setup(){
  Serial.begin(115200);
  setupWifi();
}