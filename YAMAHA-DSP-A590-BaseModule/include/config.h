#pragma once
#include <Arduino.h>

namespace Config {
    // D2 ist beim ESP8266 (D1 Mini) der GPIO 4
    constexpr uint8_t PIN_IR_SEND = D2; 
}