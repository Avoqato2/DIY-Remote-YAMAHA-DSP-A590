#pragma once
#include <Arduino.h>

namespace Network {
    void init();
    void send_command(const String& cmd);
}