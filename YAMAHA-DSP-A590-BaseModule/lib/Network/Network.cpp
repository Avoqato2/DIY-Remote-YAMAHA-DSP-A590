#include "Network.h"
#include "../../include/secrets.h"
#include "../IRControl/IRControl.h"

#include <ESP8266WiFi.h>
#include <espnow.h>

namespace Network {

    typedef struct struct_message {
        char command[32]; 
    } struct_message;

    // ESP8266 ESP-NOW Callback
    void on_data_recv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
        struct_message payload;
        memcpy(&payload, incomingData, sizeof(payload));
        
        // Befehl an den IR-Briefkasten übergeben
        IRControl::queue_command(String(payload.command));
    }

    void init() {
        Serial.println("-----start W-LAN connection-----");
        
        WiFi.mode(WIFI_STA); 
        WiFi.setAutoReconnect(true); 
        WiFi.persistent(false); 
        WiFi.setSleepMode(WIFI_NONE_SLEEP); // Wichtig für Webserver-Stabilität!
        WiFi.begin(SECRET_SSID, SECRET_PASS); 

        // Warteschleife OHNE delay() - nutzt stattdessen yield()
        unsigned long start_time = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start_time < 20000) {
            yield(); // Lässt den ESP atmen
        }

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WLAN fehlgeschlagen. Neustart!");
            ESP.restart(); 
        }

        Serial.print("Verbunden! IP: ");
        Serial.println(WiFi.localIP());

        // ESP-NOW starten
        if(esp_now_init() != 0){
            Serial.println("ESP-NOW Fehler. Neustart!");
            ESP.restart();
        }
        
        esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
        esp_now_register_recv_cb(on_data_recv);
    }
}