#include "IRControl.h"
#include "../../include/config.h"
#include <map>
#include <IRremote.hpp>

namespace IRControl {
    // Der Zwischenspeicher für ankommende Befehle
    String pending_command = "";
    volatile bool has_command = false;

    std::map<String, int> ir_codes = {
        {"standby", 31}, {"sleep", 87}, {"volume_up", 26}, {"volume_down", 27},
        {"ld/tv", 23}, {"cd", 21}, {"phono", 20}, {"video_aux", 85},
        {"tuner", 22}, {"vcr1", 15}, {"vcr2", 19}, {"effect_on_off", 86},
        {"test", 133}, {"delay_center_rear_swf", 134}, {"delay_up", 82}, {"delay_down", 83},
        {"center_up", 130}, {"center_down", 131}, {"rear_up", 94}, {"rear_down", 95},
        {"prologic", 136}, {"enhanced", 137}, {"concert_hall", 141}, {"concert_video", 138},
        {"rock_concert", 140}, {"disco", 143}, {"mono_movie", 139}, {"stadium", 142}
    };

    void init() {
        IrSender.begin(Config::PIN_IR_SEND);
    }

    // Wird von ESP-NOW oder WebSocket aufgerufen (muss blitzschnell sein!)
    void queue_command(const String& cmd) {
        noInterrupts(); // Schützt die Variable, falls ESP-NOW dazwischenfunkt
        pending_command = cmd;
        has_command = true;
        interrupts();
    }

    // Wird gemütlich im loop() abgearbeitet
    void update() {
        if (has_command) {
            String cmd_to_send;
            noInterrupts();
            cmd_to_send = pending_command;
            has_command = false;
            interrupts();

            if (ir_codes.count(cmd_to_send) > 0) {
                IrSender.sendNEC(122, ir_codes[cmd_to_send], 0);
                Serial.println("IR Gesendet: " + cmd_to_send);
            }
        }
    }
}