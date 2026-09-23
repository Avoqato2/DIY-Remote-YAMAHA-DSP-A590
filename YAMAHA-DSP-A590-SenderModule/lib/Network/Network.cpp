#include "Network.h"
#include "../../include/config.h"
#include "../../include/secrets.h"
#include "../Display/Display_Driver.h" // Für Fehlermeldungen auf dem Display

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>

namespace Network {
    RTC_DATA_ATTR int saved_channel = 0; 
    
    typedef struct struct_message {
        char command[32]; 
    } struct_message;

    struct_message myData;
    esp_now_peer_info_t peerInfo;
    
    volatile bool paket_send = false;
    volatile bool paket_succsess = false;

    // --- NEU: Sichere Wartefunktion ohne Systemblockade ---
    void safe_delay(unsigned long ms) {
        unsigned long start = millis();
        while (millis() - start < ms) { 
            yield(); // Lässt Hintergrundprozesse (WLAN, Watchdog) weiterlaufen
        }
    }

    void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
        paket_succsess = (status == ESP_NOW_SEND_SUCCESS);
        paket_send = true; 
    }

    void sync_wifi_channel() {
        Display::draw_feedback("Syncing WiFi...");
        WiFi.begin(SECRET_SSID, SECRET_PASS);
        
        int trys = 0;
        while (WiFi.status() != WL_CONNECTED && trys < 15) { 
            safe_delay(500); // Ersetzt: delay(500);
            trys++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            saved_channel = WiFi.channel(); 
            Serial.println("Neuer Kanal: " + String(saved_channel));
            WiFi.disconnect(); 
        } else {
            Serial.println("Router nicht gefunden! Fallback auf 1.");
            saved_channel = 1;
        }
    }

    void init() {
        WiFi.mode(WIFI_STA);

        if (saved_channel == 0) {
            sync_wifi_channel();
        }

        esp_wifi_set_promiscuous(true);
        esp_wifi_set_channel(saved_channel, WIFI_SECOND_CHAN_NONE);
        esp_wifi_set_promiscuous(false);

        if (esp_now_init() != ESP_OK) {
            Display::draw_feedback(" ERROR: RESTART...");
            safe_delay(1000); // Ersetzt: delay(1000);
            ESP.restart();
        }

        esp_now_register_send_cb(on_data_sent);

        memcpy(peerInfo.peer_addr, Config::BASE_MAC, 6);
        peerInfo.channel = saved_channel;  
        peerInfo.encrypt = false;
        esp_now_add_peer(&peerInfo);
    }

    void send_command(const String& cmd) {
        strncpy(myData.command, cmd.c_str(), sizeof(myData.command));
        paket_send = false;
        esp_now_send(Config::BASE_MAC, (uint8_t *) &myData, sizeof(myData));

        unsigned long start_time = millis();
        // Hier war schon das yield() drin
        while (!paket_send && millis() - start_time < 100) { yield(); }

        if (paket_succsess) {
            Serial.println("Gesendet auf Kanal " + String(saved_channel) + ": " + cmd);
            return;
        }

        // Fallback: Kanal neu suchen
        Display::draw_feedback("ERROR: Syncing WiFi...");
        sync_wifi_channel();
        
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_channel(saved_channel, WIFI_SECOND_CHAN_NONE);
        esp_wifi_set_promiscuous(false);

        peerInfo.channel = saved_channel;
        esp_now_mod_peer(&peerInfo); 

        paket_send = false;
        esp_now_send(Config::BASE_MAC, (uint8_t *) &myData, sizeof(myData));

        start_time = millis();
        // Hier auch yield()
        while (!paket_send && millis() - start_time < 100) { yield(); }

        if (paket_succsess) {
            Serial.println("Nach Sync gesendet: " + cmd);
        } else {
            Display::draw_feedback("Error: Base Offline");
        }
    }
}