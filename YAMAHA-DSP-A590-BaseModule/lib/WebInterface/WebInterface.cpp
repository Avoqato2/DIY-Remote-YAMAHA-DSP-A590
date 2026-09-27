#include "WebInterface.h"
#include "../../include/remote_html.h"

#include "../IRControl/IRControl.h"

#include <Arduino.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>

namespace WebInterface {

    AsyncWebServer server(80);
    AsyncWebSocket ws("/ws");

    bool mdns_started = false;

    void on_event(
        AsyncWebSocket* server,
        AsyncWebSocketClient* client,
        AwsEventType type,
        void* arg,
        uint8_t* data,
        size_t len
    )
    {
        if (type == WS_EVT_CONNECT) {
            Serial.printf("WebSocket verbunden. Aktiv: %u\n", ws.count());
            return;
        }

        if (type == WS_EVT_DISCONNECT) {
            Serial.printf("WebSocket Client getrennt. Aktiv: %u\n", ws.count());
            return;
        }

        if (type == WS_EVT_DATA) {
            char command[32];

            size_t size = min(
                len,
                sizeof(command) - 1
            );

            memcpy(command, data, size);
            command[size] = '\0';

            IRControl::queue_command(command);
        }
    }

    bool start_mdns()
    {
        if (mdns_started)
            return true;

        if (WiFi.status() != WL_CONNECTED)
            return false;

        if (!MDNS.begin("amp-remote"))
            return false;

        MDNS.addService("http", "tcp", 80);

        mdns_started = true;

        Serial.println("mDNS: amp-remote.local");

        return true;
    }

    void stop_mdns()
    {
        if (!mdns_started)
            return;

        MDNS.close();
        mdns_started = false;
    }

    void init()
    {
        server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
            AsyncWebServerResponse* response =
                request->beginResponse_P(
                    200,
                    "text/html",
                    HTML
                );

            response->addHeader("Connection", "close");
            request->send(response);
        });

        ws.onEvent(on_event);
        server.addHandler(&ws);
        server.begin();

        start_mdns();
    }

    void update()
    {
        static unsigned long last_cleanup = 0;
        static unsigned long last_mdns_retry = 0;

        if (mdns_started) {
            MDNS.update();
        }
        else if (
            WiFi.status() == WL_CONNECTED &&
            millis() - last_mdns_retry > 5000
        ) {
            last_mdns_retry = millis();
            start_mdns();
        }

        if (millis() - last_cleanup > 1000) {
            ws.cleanupClients();
            last_cleanup = millis();
        }
    }

    void wifi_lost()
    {
        stop_mdns();
    }

    void wifi_connected()
    {
        start_mdns();
    }

}