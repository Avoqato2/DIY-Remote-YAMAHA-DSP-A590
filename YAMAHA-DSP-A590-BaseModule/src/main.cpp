#include <Arduino.h>
#include <IRremote.hpp>

const int IR_SEND_PIN = D4; // Der Pin für die Infrarot-LED
const int volumeUpCode = 27; // Dein Code für "Lauter"

void setup() {
  Serial.begin(115200);
  delay(1000); // Kurz warten, bis der Serielle Monitor bereit ist
  
  // IR-Sender initialisieren
  IrSender.begin(IR_SEND_PIN);
  
  Serial.println("Starte Dauerfeuer: Lauter...");
}

void loop() {
  // 1. Sende den NEC Code (Adresse 122, Command 26)
  IrSender.sendNEC(122, volumeUpCode, 0);
  
  // 2. Gib eine Nachricht im Seriellen Monitor aus
  Serial.println("Feuere Lauter-Code");
  
  // 3. Warte 100 Millisekunden (wie eine echte Fernbedienung) und wiederhole es
  delay(100); 
}