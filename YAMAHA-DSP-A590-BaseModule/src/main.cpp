#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <IRremote.hpp>

const char* ssid = "VOR-Verstärker";
const char* password = "***REMOVED***";
const int IR_SEND_PIN = D4; 
const int volUpCode = 26;
const int volDownCode = 27; // Nimm hier den echten Code für "Leiser"
const int standbyCode = 28; // Nimm hier den echten Code für "Standby"

// --- SERVER & STATUS ---
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

int currentSliderValue = 50; // Startwert in der Mitte

// --- HTML FRONTEND (Wird direkt im Chip gespeichert) ---
const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>AMP-Remote</title>
  </head>
  <body>
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

      /* Verhindert jegliches Scrollen und setzt die Höhe auf 100% des Viewports */
      html,
      body {
        margin: 0;
        padding: 0;
        height: 100%;
        width: 100%;
        overflow: hidden;
        background-color: #121212;
      }

      * {
        box-sizing: border-box;
      }

      /* Der Body als Fernbedienungs-Gehäuse, passt sich dynamisch der Bildschirmhöhe an (dvh) */
      body {
        font-family:
          -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica,
          Arial, sans-serif;
        background-color: var(--remote-bg);
        color: var(--btn-text);
        max-width: 400px;
        height: 96dvh; /* Nutzt 96% der sichtbaren Bildschirmhöhe */
        margin: 2dvh auto;
        padding: 2dvh 15px;
        border-radius: 40px;
        border: 2px solid var(--remote-border);
        box-shadow:
          0 20px 50px var(--shadow),
          inset 0 4px 10px rgba(255, 255, 255, 0.05),
          inset 0 -4px 10px rgba(0, 0, 0, 0.6);
        display: flex;
        flex-direction: column;
        justify-content: space-between; /* Verteilt alle Sektionen gleichmäßig über die Höhe */
      }

      /* Typografie skaliert dynamisch mit der Bildschirmhöhe */
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

      /* Alle Container verteilen ihren Platz dynamisch */
      div {
        display: flex;
        flex-wrap: wrap;
        width: 100%;
        justify-content: center;
        padding: 0.5dvh 0;
        gap: 1dvh; /* Abstände skalieren mit der Höhe */
      }

      /* Tasten passen ihre Höhe automatisch an den verfügbaren Platz an */
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
        box-shadow:
          0 4px 6px rgba(0, 0, 0, 0.4),
          inset 0 2px 2px var(--highlight);
        transition:
          transform 0.05s,
          box-shadow 0.05s,
          background-color 0.1s;
        display: flex;
        justify-content: center;
        align-items: center;
        text-align: center;
        padding: 0; /* Padding entfernt, damit Flexbox die Höhe komplett steuert */
        min-height: 4.5dvh; /* Mindesthöhe für Tappbarkeit am Handy */
      }

      button:active {
        transform: translateY(2px);
        background-color: var(--btn-active);
        box-shadow:
          0 1px 2px rgba(0, 0, 0, 0.6),
          inset 0 2px 4px rgba(0, 0, 0, 0.4);
      }

      /* --- SPEZIFISCHE SEKTIONEN --- */

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

      /* Lautstärke jetzt nebeneinander, um stark vertikalen Platz zu sparen */
      #volume_control_sec {
        flex-direction: row;
        gap: 1dvh;
      }

      #volume_control_sec button {
        flex: 1;
        font-size: min(1.1rem, 2.5dvh);
        border-radius: 16px;
        background-color: #33363a;
        min-height: 5dvh; /* Wichtige Tasten etwas höher */
      }

      /* Kanäle (3er-Grid) */
      #channel_control_sec {
        justify-content: space-between;
      }

      #channel_control_sec button {
        flex: 1 1 calc(33.333% - 1dvh);
        border-radius: 20px;
        font-size: min(0.8rem, 1.8dvh);
      }

      /* Effekte (Verschachtelte Layouts) */
      #effect_control_sec {
        flex-direction: column;
        flex: 1; /* Nimmt den restlichen Platz ein */
        justify-content: space-between;
      }

      #effect_control_on_off button {
        width: 100%;
        border-radius: 12px;
        min-height: 5dvh;
      }

      /* Der Menü-Block für die Pegel-Einstellung */
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

      /* Effekt-Typen (2er-Grid) */
      #effect_controlType {
        justify-content: space-between;
      }

      #prologic_enhanced {
        width: 100%;
        justify-content: space-between;
      }

      #effect_controlType button,
      #prologic_enhanced button {
        flex: 1 1 calc(50% - 0.5dvh);
        font-size: min(0.8rem, 1.8dvh);
        border-radius: 8px;
      }

      /* --- MOBILE ANPASSUNG --- */
      @media (max-width: 420px) {
        html,
        body {
          background-color: var(--remote-bg);
        }
        body {
          max-width: 100%;
          height: 100dvh;
          margin: 0;
          border-radius: 0;
          border: none;
          box-shadow: none;
          padding: 3dvh 15px; /* Etwas mehr Randabstand am Handy oben/unten */
        }
      }
    </style>
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
        <button id="delay_center_rear_swf">Delay/Center/Rear/SWF</button>
        <div id="delay_center_rear_swf_level">
          <button id="delay_center_rear_swf_up">Level +</button>
          <button id="delay_center_rear_swf_down">Level -</button>
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

      // Sobald die Seite lädt, WebSocket starten
      window.addEventListener("load", function () {
        websocket = new WebSocket(gateway);

        // Listener
        document.addEventListener("click", function (event) {
          if (event.target.tagName === "BUTTON") {
            const button_id = event.target.id;
            sendAction(button_id);
            console.log(`Button clicked: ${button_id}`);
          }
        });
      });

      // Funktion für normale Knöpfe
      function sendAction(action) {
        websocket.send(action);
      }
    </script>
  </body>
</html>
)rawliteral";


// --- WEBSOCKET LOGIK (Hier passiert die Magie) ---
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    // Die empfangenen Daten in einen lesbaren String verwandeln
    data[len] = 0;
    String msg = (char*)data;
    
    Serial.print("WebSocket empfangen: ");
    Serial.println(msg);

    // 1. Wenn die Nachricht vom Slider kommt (z.B. "SLIDER:65")
    if (msg.startsWith("SLIDER:")) {
      int newVal = msg.substring(7).toInt(); // Schneidet "SLIDER:" ab und macht eine Zahl daraus
      int diff = newVal - currentSliderValue;
      
      if (diff > 0) {
        // Muss lauter werden
        for(int i = 0; i < diff; i++) {
          IrSender.sendNEC(122, volUpCode, 0);
          delay(40); // WICHTIG: Kurze Pause, sonst verschluckt sich der Verstärker!
        }
      } 
      else if (diff < 0) {
        // Muss leiser werden
        for(int i = 0; i < abs(diff); i++) {
          IrSender.sendNEC(122, volDownCode, 0);
          delay(40); 
        }
      }
      
      currentSliderValue = newVal; // Neuen Stand merken
    }
    
    // 2. Wenn der Standby-Knopf gedrückt wurde
    else if (msg == "STANDBY") {
      IrSender.sendNEC(122, standbyCode, 0);
    }
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  delay(2000); 
  
  IrSender.begin(IR_SEND_PIN);
  
  Serial.print("\nVerbinde mit WLAN: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Warteschleife
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nErfolgreich verbunden!");
  Serial.print("-> Öffne im Browser: http://");
  Serial.println(WiFi.localIP());

  // Die HTML Seite auf der Hauptroute (/) ausliefern
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Den WebSocket-Tunnel anbinden
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  server.begin();
}

void loop() {
  // Der Loop bleibt leer! AsyncWebServer macht alles im Hintergrund.
}