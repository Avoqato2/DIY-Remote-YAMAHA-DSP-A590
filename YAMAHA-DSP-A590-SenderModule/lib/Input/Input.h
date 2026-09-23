#pragma once
#include <Arduino.h>

namespace Input {
    void init();
    void update(); // Muss im loop() laufen!
    
    int get_and_clear_right_turns();
    int get_and_clear_left_turns();
    
    // Taster-Abfragen (geben nur 1x true zurück, wenn gedrückt)
    bool is_up_pressed();
    bool is_down_pressed();
    bool is_sw_pressed();
    bool is_power_pressed();
    bool has_activity(); // True, wenn IRGENDWAS gedrückt/gedreht wurde
}