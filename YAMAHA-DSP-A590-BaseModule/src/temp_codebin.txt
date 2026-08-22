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

  // WLAN Credentials
  const char* ssid = SECRET_SSID;
  const char* password = SECRET_PASS;



  // --- LOG-SYSTEM ---
  String logBuffer = ""; // Hier speichern wir die Log-Einträge

  void addLog(String message) {
    // Berechne Laufzeit in Stunden, Minuten, Sekunden
    unsigned long secs = millis() / 1000;
    String timeStr = "[" + String(secs / 3600) + ":" + String((secs / 60) % 60) + ":" + String(secs % 60) + "] ";
    
    String newEntry = timeStr + message;
    Serial.println(newEntry); // Weiterhin über USB ausgeben
    
    // Im RAM speichern (mit HTML-Zeilenumbruch)
    logBuffer += newEntry + "<br>";

    // Verhindern, dass der Arbeitsspeicher überläuft (maximal ~2000 Zeichen behalten)
    if (logBuffer.length() > 2000) {
      int cutPos = logBuffer.indexOf("<br>") + 4;
      logBuffer = logBuffer.substring(cutPos);
    }
  }

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

  // --- SERVER & STATUS ---
  AsyncWebServer server(80);
  AsyncWebSocket ws("/ws");

  unsigned long letzterWlanCheck = 0; 

  // --- ESP-NOW DATENSTRUKTUR & PUFFER ---
  typedef struct struct_message {
      char command[32]; 
  } struct_message;

  // Für sichere Übergabe vom Callback an den Hauptloop
  volatile bool newEspNowCommand = false;
  char pendingCommand[32] = {0};

  // --- ESP-NOW EMPFANGS-LOGIK (Nur sichere Übergabe, keine Speicherallokation) ---
  void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
    struct_message payload;
    memcpy(&payload, incomingData, sizeof(payload));
    
    strncpy(pendingCommand, payload.command, sizeof(pendingCommand) - 1);
    pendingCommand[sizeof(pendingCommand) - 1] = '\0';
    newEspNowCommand = true;
  }

  // --- HTML FRONTEND (1:1 unverändert) ---

  // --- WEBSOCKET LOGIK ---
  void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
      addLog("Web-Client verbunden: IP " + client->remoteIP().toString());
    } else if (type == WS_EVT_DISCONNECT) {
      addLog("Web-Client getrennt");
    } else if (type == WS_EVT_DATA) {
      data[len] = 0;
      String msg = (char*)data;
      
      if (ir_codes.count(msg) > 0) {
        IrSender.sendNEC(122, ir_codes[msg], 0);
        addLog("Web-Befehl: " + msg);
      }
    }
  }

  // --- SETUP ---
  void setup() {
    logBuffer.reserve(2500); // 1. Speicherbereich fest reservieren gegen Heap-Fragmentierung

    Serial.begin(115200);
    delay(2000); 
    
    IrSender.begin(IR_SEND_PIN);
    
    addLog("System gestartet.");
    addLog("Verbinde mit WLAN: " + String(ssid));
    
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true); 
    WiFi.persistent(false);      
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFi.begin(ssid, password);

    int versuche = 0;
    while (WiFi.status() != WL_CONNECTED && versuche < 40) {
      delay(500);
      Serial.print(".");
      versuche++;
    }

    if (WiFi.status() != WL_CONNECTED) {
      addLog("WLAN-Timeout! Modul startet neu...");
      delay(1000);
      ESP.restart(); 
    }

    addLog("Erfolgreich verbunden! IP: " + WiFi.localIP().toString());
    letzterWlanCheck = millis();

    // --- ESP-NOW SETUP ---
    if (esp_now_init() != 0) {
      addLog("Fehler bei ESP-NOW Init!");
    } else {
      esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
      esp_now_register_recv_cb(OnDataRecv);
      addLog("ESP-NOW bereit. MAC: " + WiFi.macAddress());
    }

    // Route 1: Die normale Fernbedienung
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", HTML);
    });

    // Route 2: Das System-Logbuch
    server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request){
      String html = "<html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='10'></head>";
      html += "<body style='font-family:monospace; background-color:#121212; color:#0f0;'>";
      html += "<h2>System Logbuch</h2>";
      html += logBuffer;
      html += "</body></html>";
      request->send(200, "text/html", html);
    });

    ws.onEvent(onEvent);
    server.addHandler(&ws);

    server.begin();
    addLog("Webserver gestartet.");
  }

  unsigned long letzterLoopCheck = 0;

  // --- HAUPTSCHLEIFE ---
  void loop() {
    // 2. ESP-NOW Befehl sofort im Hauptkontext ohne Blockade abarbeiten
    if (newEspNowCommand) {
      newEspNowCommand = false;
      String msg = String(pendingCommand);
      
      if (ir_codes.count(msg) > 0) {
        IrSender.sendNEC(122, ir_codes[msg], 0);
        addLog("ESP-NOW Empfang: " + msg + " -> IR gesendet"); 
      } else {
        addLog("ESP-NOW Fehler: Unbekannter Befehl '" + msg + "'");
      }
    }

    unsigned long jetzt = millis();

    // Wartungs-Block alle 2 Sekunden
    if (jetzt - letzterLoopCheck >= 2000) {
      ws.cleanupClients(); // Tote Verbindungen kappen
      
      // 3. WLAN-Wächter (Überlässt ESP den Reconnect; greift nur bei 5 Min Totalausfall hart ein)
      if (WiFi.status() != WL_CONNECTED) {
        if (jetzt - letzterWlanCheck >= 300000) { // 5 Minuten
          addLog("WLAN-Verbindung dauerhaft verloren! Not-Neustart...");
          delay(1000);
          ESP.restart();
        }
      } else {
        letzterWlanCheck = jetzt;
      }
      
      letzterLoopCheck = jetzt;
    }
  }