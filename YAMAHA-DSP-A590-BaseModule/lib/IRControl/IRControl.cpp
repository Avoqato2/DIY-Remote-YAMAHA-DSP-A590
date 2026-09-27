#include "IRControl.h"
#include "../../include/config.h"

#include <Arduino.h>
#include <IRremote.hpp>
#include <map>

namespace IRControl {

    char pending_command[32] = {0};
    volatile bool has_command = false;

    std::map<String, int> ir_codes = {
        {"standby", 31},
        {"sleep", 87},
        {"volume_up", 26},
        {"volume_down", 27},
        {"ld/tv", 23},
        {"cd", 21},
        {"phono", 20},
        {"video_aux", 85},
        {"tuner", 22},
        {"vcr1", 15},
        {"vcr2", 19},
        {"effect_on_off", 86},
        {"test", 133},
        {"delay_center_rear_swf", 134},
        {"delay_up", 82},
        {"delay_down", 83},
        {"center_up", 130},
        {"center_down", 131},
        {"rear_up", 94},
        {"rear_down", 95},
        {"prologic", 136},
        {"enhanced", 137},
        {"concert_hall", 141},
        {"concert_video", 138},
        {"rock_concert", 140},
        {"disco", 143},
        {"mono_movie", 139},
        {"stadium", 142}
    };

    void init()
    {
        IrSender.begin(Config::PIN_IR_SEND);
    }

    void queue_command(const char* cmd)
    {
        if (!cmd)
            return;

        noInterrupts();

        strncpy(
            pending_command,
            cmd,
            sizeof(pending_command) - 1
        );

        pending_command[sizeof(pending_command) - 1] = '\0';
        has_command = true;

        interrupts();
    }

    void update()
    {
        if (!has_command)
            return;

        char command[32];

        noInterrupts();
        strcpy(command, pending_command);
        has_command = false;
        interrupts();

        String cmd = command;

        auto it = ir_codes.find(cmd);

        if (it == ir_codes.end()) {
            Serial.println("Unbekannter IR Befehl: " + cmd);
            return;
        }

        IrSender.sendNEC(122, it->second, 0);

        Serial.println("IR Gesendet: " + cmd);
    }

}