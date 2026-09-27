#include "Network.h"
#include "../../include/secrets.h"

#include "../IRControl/IRControl.h"
#include "../WebInterface/WebInterface.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

namespace Network {

    struct struct_message {
        char command[32];
    };

    bool was_connected = false;

    void on_data_recv(
        uint8_t* mac,
        uint8_t* incomingData,
        uint8_t len
    )
    {
        struct_message payload = {};

        size_t size = min(
            (size_t)len,
            sizeof(payload.command) - 1
        );

        memcpy(payload.command, incomingData, size);
        payload.command[size] = '\0';

        IRControl::queue_command(payload.command);
    }

    void init()
    {
        Serial.println("Starte WLAN...");

        WiFi.mode(WIFI_STA);
        WiFi.setSleepMode(WIFI_NONE_SLEEP);
        WiFi.setAutoReconnect(true);
        WiFi.persistent(false);
        WiFi.setPhyMode(WIFI_PHY_MODE_11G);

        WiFi.begin(SECRET_SSID, SECRET_PASS);

        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
            Serial.println("WLAN fehlgeschlagen. Neustart.");
            delay(500);
            ESP.restart();
        }

        Serial.print("WLAN verbunden: ");
        Serial.println(WiFi.localIP());

        if (esp_now_init() != 0) {
            Serial.println("ESP-NOW Fehler. Neustart.");
            delay(500);
            ESP.restart();
        }

        esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
        esp_now_register_recv_cb(on_data_recv);

        was_connected = true;
    }

   void update()
    {
        static unsigned long last_check = 0;

        if (millis() - last_check < 1000)
            return;

        last_check = millis();

        bool connected = (WiFi.status() == WL_CONNECTED);

        if (!connected && was_connected) {
            Serial.println("WLAN verloren.");
            
            // mDNS sicher stoppen
            WebInterface::wifi_lost();
            
            was_connected = false;
        }

        if (connected && !was_connected) {
            Serial.println("WLAN wieder verbunden.");
            Serial.println(WiFi.localIP());

            was_connected = true;

            // mDNS wieder hochfahren
            WebInterface::wifi_connected();
        }
    }

}