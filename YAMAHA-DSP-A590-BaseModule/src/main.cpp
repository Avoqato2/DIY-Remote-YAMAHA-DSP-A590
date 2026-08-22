  #include <Arduino.h>
  #include <ESP8266WiFi.h>        // Specific ESP32 D1 Mini wfif-lib
  #include <ESPAsyncTCP.h>        // Async lib for javascript i think
  #include <ESPAsyncWebServer.h>  // Webserver lib
  #include <IRremote.hpp>         // Infrared lib
  #include <map>                  // Map lib for organizing IR-Codes
  #include <espnow.h>             // esp now Lib
//_________________________________________________________________________________
//_________________________________________________________________________________
// -------------------------extern file imports------------------------------------
  #include "secrets.h"
  #include "remote_html.h"
//_________________________________________________________________________________
//_________________________________________________________________________________
//_________________________________________________________________________________
//_________________________________________________________________________________
// --------------------------------------------------------------------------------
// ------------------------------Define Pins---------------------------------------
// --------------------------------------------------------------------------------
  // Infrared pin for the ESP8266 D1 Mini (GPIO2)
  const int IR_SEND_PIN = D2; 
// --------------------------------------------------------------------------------
// ------------------------------Define Pins---------------------------------------
// --------------------------------------------------------------------------------
//_________________________________________________________________________________
//_________________________________________________________________________________
// --------------------------------------------------------------------------------
// -----------------------------W-LAN Section--------------------------------------
// --------------------------------------------------------------------------------

 // WLAN Credentials
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

void wlan_setup(){
    Serial.println("-----start W-LAN connection-----");
  
    WiFi.mode(WIFI_STA);  // connect to WLAN like a Phone for an IP-Adress
    WiFi.setAutoReconnect(true);  // ESP reconnect automaticly
    WiFi.persistent(false); // prevents that esp saves pwd and ssid in flash drive.
    WiFi.setSleepMode(WIFI_NONE_SLEEP); // prevents that the esp lets the wifi module go in to sleep mode, 
                                        //so that it is always availble with no delay
    WiFi.begin(ssid,password);  // connection with SSID, PWD
    // Stay in while till we are connected
    int trys = 0;
    while (WiFi.status() != WL_CONNECTED && trys < 40) {
      delay(500);
      Serial.print(".");
      trys++;
    }

    // Restart if the Connection is invalide
    if (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      ESP.restart(); 
    }

    Serial.println();
    Serial.println("Succesfully connected!");
    Serial.print("IP-Adress: ");
    Serial.println(WiFi.localIP());
}
// --------------------------------------------------------------------------------
// -----------------------------W-LAN Section--------------------------------------
// --------------------------------------------------------------------------------

  // Infrared codes for AMP
  std::map<String, int> ir_codes = {
    {"standby", 31}, {"sleep", 87}, {"volume_up", 26}, {"volume_down", 27},
    {"ld/tv", 23}, {"cd", 21}, {"phono", 20}, {"video_aux", 85},
    {"tuner", 22}, {"vcr1", 15}, {"vcr2", 19}, {"effect_on_off", 86},
    {"test", 133}, {"delay_center_rear_swf", 134}, {"delay_up", 82}, {"delay_down", 83},
    {"center_up", 130}, {"center_down", 131}, {"rear_up", 94}, {"rear_down", 95},
    {"prologic", 136}, {"enhanced", 137}, {"concert_hall", 141}, {"concert_video", 138},
    {"rock_concert", 140}, {"disco", 143}, {"mono_movie", 139}, {"stadium", 142}
  };
//_________________________________________________________________________________
//_________________________________________________________________________________
// --------------------------------------------------------------------------------
// -----------------------Webserver&Websocket Section------------------------------
// --------------------------------------------------------------------------------

  AsyncWebServer server(80); // start Webserver on port standard 80
  AsyncWebSocket ws("/ws"); 

  //  AsyncWebSocket *server        ->  ....
  //  AsyncWebSocketClient *client  ->  Who is causing the event
  //  AwsEventType type             ->  What happend? Connect, disconnet, Message
  //  void *arg                     ->  reseved memory for Big transfers, it does not matter, but it has to be in () for the complier
  //  uint8_t *data                 ->  data as in what comes for a mesage
  //  size_t len                    ->  How long is the message
 void on_event(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {           //do stuff on connedtion
      Serial.println("client is connected");
    } 
    else if (type == WS_EVT_DISCONNECT) {   // do stuff on disconnection
      Serial.println("client disconnected");
    } 
    else if (type == WS_EVT_DATA) {   // do stuff on data
      data[len] = 0;
      String msg = (char*)data;
      
      if (ir_codes.count(msg) > 0) {
        IrSender.sendNEC(122, ir_codes[msg], 0);
        Serial.println("Reseved CMD:"+ msg);
      }
    }
  }

// --------------------------------------------------------------------------------
// -----------------------Webserver&Websocket Section------------------------------
// --------------------------------------------------------------------------------
  void setup() {
    Serial.begin(115200);
    delay(2000); // Wait till Serial monitor is Ready
    //start W-Lan Connection
    wlan_setup();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", HTML);
    });

    ws.onEvent(on_event);    // giving the on Evnet as callback
    server.addHandler(&ws); // & symbol means that the server gets the adress for the Websocket and not the whole object

    server.begin();
  }


  void loop() {
   ws.cleanupClients();
  }