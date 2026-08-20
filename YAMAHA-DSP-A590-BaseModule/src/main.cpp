#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <IRremote.hpp>
#include <map>
#include <espnow.h> 

// WLAN Credentials
const char* ssid = "VOR-Verstärker";
const char* password = "***REMOVED***";

// Infrared pin for the ESP8266 D1 Mini (GPIO2)
const int IR_SEND_PIN = D2; 

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
const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>AMP-Remote</title>
    <style>
      :root {
        --remote-bg: #222224;
        --remote-border: #111;
        --btn-base: #3b3d40;
        --btn-active: #2a2b2d;
        --btn-text: #eaeaea;
        --accent-red: #c9302c;
        --accent-red-active: #a02622;
        --highlight: rgba(255, 255, 255, 0.1);
        --shadow: rgba(0, 0, 0, 0.5);
      }

      html, body {
        margin: 0;
        padding: 0;
        height: 100%;
        width: 100%;
        overflow: hidden;
        background-color: #121212;
        touch-action: manipulation;
      }

      * {
        box-sizing: border-box;
      }

      body {
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
        background-color: var(--remote-bg);
        color: var(--btn-text);
        max-width: 400px;
        height: 96dvh;
        margin: 2dvh auto;
        padding: 2dvh 15px;
        border-radius: 40px;
        border: 2px solid var(--remote-border);
        box-shadow: 0 20px 50px var(--shadow), 
                    inset 0 4px 10px rgba(255, 255, 255, 0.05), 
                    inset 0 -4px 10px rgba(0, 0, 0, 0.6);
        display: flex;
        flex-direction: column;
        justify-content: space-between;
      }

      h1 {
        font-size: min(1.4rem, 3.5dvh);
        margin: 0;
        font-weight: 700;
        color: #888;
        text-transform: uppercase;
        letter-spacing: 2px;
        text-align: center;
        flex-shrink: 0;
      }

      h2 {
        font-size: min(0.85rem, 2.2dvh);
        color: #777;
        text-transform: uppercase;
        letter-spacing: 1px;
        width: 100%;
        margin: 0;
        border-bottom: 1px solid #333;
        padding-bottom: 0.5dvh;
        flex-shrink: 0;
      }

      div {
        display: flex;
        flex-wrap: wrap;
        width: 100%;
        justify-content: center;
        padding: 0.5dvh 0;
        gap: 1dvh;
      }

      button {
        background-color: var(--btn-base);
        color: var(--btn-text);
        border: none;
        border-radius: 10px;
        font-size: min(0.9rem, 2dvh);
        font-weight: 600;
        cursor: pointer;
        user-select: none;
        -webkit-tap-highlight-color: transparent;
        box-shadow: 0 4px 6px rgba(0, 0, 0, 0.4), 
                    inset 0 2px 2px var(--highlight);
        transition: transform 0.05s, box-shadow 0.05s, background-color 0.1s;
        display: flex;
        justify-content: center;
        align-items: center;
        text-align: center;
        padding: 0;
        min-height: 4.5dvh;
      }

      button:active {
        transform: translateY(2px);
        background-color: var(--btn-active);
        box-shadow: 0 1px 2px rgba(0, 0, 0, 0.6), 
                    inset 0 2px 4px rgba(0, 0, 0, 0.4);
      }

      #main_button_sec {
        justify-content: space-between;
      }

      #main_button_sec button {
        flex: 0 1 calc(50% - 0.5dvh);
        border-radius: 20px;
        margin-bottom: 1dvh;
      }

      #standby {
        background-color: var(--accent-red);
        color: white;
      }

      #standby:active {
        background-color: var(--accent-red-active);
      }

      #volume_control_sec {
        flex-direction: row;
        gap: 1dvh;
      }

      #volume_control_sec button {
        flex: 1;
        font-size: min(1.1rem, 2.5dvh);
        border-radius: 16px;
        background-color: #33363a;
        min-height: 5dvh;
      }

      #channel_control_sec {
        justify-content: space-between;
      }

      #channel_control_sec button {
        flex: 1 1 calc(33.333% - 1dvh);
        border-radius: 20px;
        font-size: min(0.8rem, 1.8dvh);
      }

      #effect_control_sec {
        flex-direction: column;
        flex: 1;
        justify-content: space-between;
      }

      #effect_control_on_off button {
        width: 100%;
        border-radius: 12px;
        min-height: 5dvh;
      }

      #delay_center_rear_swf_control_sec {
        background-color: #1a1a1c;
        padding: 1dvh;
        border-radius: 12px;
        border: 1px solid #333;
      }

      #delay_center_rear_swf_control_sec > button {
        flex: 1 1 100%;
      }

      #delay_center_rear_swf_level {
        flex-direction: row;
        margin-top: 0.5dvh;
      }

      #delay_center_rear_swf_level button {
        flex: 1 1 calc(50% - 0.5dvh);
      }

      #effect_controlType {
        justify-content: space-between;
      }

      #prologic_enhanced {
        width: 100%;
        justify-content: space-between;
      }

      #effect_controlType button, #prologic_enhanced button {
        flex: 1 1 calc(50% - 0.5dvh);
        font-size: min(0.8rem, 1.8dvh);
        border-radius: 8px;
      }

      @media (max-width: 420px) {
        html, body {
          background-color: var(--remote-bg);
        }
        body {
          max-width: 100%;
          height: 100dvh;
          margin: 0;
          border-radius: 0;
          border: none;
          box-shadow: none;
          padding: 3dvh 15px;
        }
      }
    </style>
  </head>
  <body>
    <h1>AMP Remote</h1>
    
    <div id="main_button_sec">
      <button id="standby">Standby</button>
      <button id="sleep">Sleep</button>
    </div>
    
    <div id="volume_control_sec">
      <button id="volume_up">Volume +</button>
      <button id="volume_down">Volume -</button>
    </div>
    
    <h2>Channels</h2>
    <div id="channel_control_sec">
      <button id="ld/tv">LD/TV</button>
      <button id="cd">CD</button>
      <button id="phono">Phono</button>
      <button id="video_aux">Video Aux</button>
      <button id="tuner">Tuner</button>
      <button id="vcr1">VCR 1</button>
      <button id="vcr2">VCR 2</button>
    </div>
    
    <h2>Effects</h2>
    <div id="effect_control_sec">
      <div id="effect_control_on_off">
        <button id="effect_on_off">Effect On/Off</button>
      </div>
      
      <div id="delay_center_rear_swf_control_sec">
        <button id="test">Test</button>
        <div id="delay_center_rear_swf_level">
          <button id="delay_up">Delay +</button>
          <button id="delay_down">Delay -</button>
          <button id="center_up">Center +</button>
          <button id="center_down">Center -</button>
          <button id="rear_up">Rear +</button>
          <button id="rear_down">Rear -</button>
        </div>
      </div>
      
      <div id="effect_controlType">
        <div id="prologic_enhanced">
          <button id="prologic">Prologic</button>
          <button id="enhanced">Enhanced</button>
        </div>
        <button id="concert_hall">Concert Hall</button>
        <button id="concert_video">Concert Video</button>
        <button id="rock_concert">Rock Concert</button>
        <button id="disco">Disco</button>
        <button id="mono_movie">Mono Movie</button>
        <button id="stadium">Stadium</button>
      </div>
    </div>
    
    <script>
      var gateway = `ws://${window.location.hostname}/ws`;
      var websocket; 
      var holdInterval; 
      
      window.addEventListener("load", function () {
        websocket = new WebSocket(gateway);
        
        const startEvents = ["mousedown", "touchstart"];
        const stopEvents = ["mouseup", "mouseleave", "touchend", "touchcancel"];
        
        startEvents.forEach(function (eventType) {
          document.addEventListener(eventType, function (event) {
            if (event.target.tagName === "BUTTON") {
              if (eventType === "touchstart") event.preventDefault();
              
              const button_id = event.target.id;
              sendAction(button_id);
              
              holdInterval = setInterval(function () { 
                sendAction(button_id); 
              }, 150);
            }
          }, { passive: false });
        });
        
        stopEvents.forEach(function (eventType) {
          document.addEventListener(eventType, function (event) {
            if (event.target.tagName === "BUTTON") {
              clearInterval(holdInterval);
            }
          });
        });
      });
      
      function sendAction(action) { 
        if (websocket.readyState === WebSocket.OPEN) {
          websocket.send(action); 
        }
      }
    </script>
  </body>
</html>
)rawliteral";

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
    request->send_P(200, "text/html", index_html);
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