#pragma once
#include <Arduino.h>

namespace IRControl {
    void init();
    void update(); 
    void queue_command(const String& cmd); // Der "Briefkasten"
}