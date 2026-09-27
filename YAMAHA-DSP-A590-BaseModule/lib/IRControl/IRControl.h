#pragma once
#include <Arduino.h>

namespace IRControl {

    void init();
    void queue_command(const char* cmd);
    void update();

}