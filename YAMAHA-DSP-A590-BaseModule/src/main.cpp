#include <Arduino.h>
#include "../lib/IRControl/IRControl.h"
#include "../lib/Network/Network.h"
#include "../lib/WebInterface/WebInterface.h"

void setup() {
    Serial.begin(115200);
    
    // Die Reihenfolge ist wichtig!
    IRControl::init();
    Network::init();      // Verbindet WLAN & ESP-NOW
    WebInterface::init(); // Startet Server & mDNS
}

void loop() {
    // 1. Schaut nach, ob IR-Befehle im Briefkasten liegen und sendet sie
    IRControl::update();
    
    // 2. Hält mDNS am Leben und räumt tote Web-Clients auf
    WebInterface::update();

    Network::update();
}