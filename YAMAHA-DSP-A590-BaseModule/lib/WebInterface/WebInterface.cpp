#include "WebInterface.h"
#include "../../include/remote_html.h"
#include "../IRControl/IRControl.h"

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>

namespace WebInterface {
    AsyncWebServer server(80);
    AsyncWebSocket ws("/ws");
    unsigned long last_cleanup = 0;

    void on_event(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if (type == WS_EVT_CONNECT) {
            if (ws.count() > 3) {
                client->close();
                return;
            }
            Serial.println("Client verbunden");
        } 
        else if (type == WS_EVT_DATA) { 
            // BLITZSCHNELL: Daten in String wandeln und sofort ab in den Briefkasten
            data[len] = 0;
            String msg = (char*)data;
            
            // NIEMALS HIER IR-SENDEN! Nur queuen:
            IRControl::queue_command(msg);
        }
    }

    void init() {
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
            AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML);
            response->addHeader("Connection", "close");
            request->send(response);
        });

        ws.onEvent(on_event);
        server.addHandler(&ws);
        server.begin();
        
        if (MDNS.begin("amp-remote")) {
            Serial.println("mDNS gestartet: amp-remote.local");
        }
    }

    void update() {
        MDNS.update(); // Für ESP8266 zwingend erforderlich
        
        // Cleanup nicht jeden Loop-Durchlauf, sondern nur 1x pro Sekunde
        if (millis() - last_cleanup > 1000) {
            ws.cleanupClients();
            last_cleanup = millis();
        }
    }
}