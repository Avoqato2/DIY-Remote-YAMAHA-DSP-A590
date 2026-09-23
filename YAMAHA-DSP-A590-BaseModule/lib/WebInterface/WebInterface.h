#pragma once
#include <Arduino.h>

namespace WebInterface {
    void init();
    void update(); // Muss im loop() laufen
}