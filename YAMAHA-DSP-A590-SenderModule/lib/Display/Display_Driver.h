#pragma once
#include <Arduino.h>

namespace Display {
    void init(bool cold_boot);
    void update();
    void sleep();
    
    void draw_header();
    void draw_menu_item(int y, const String& text, bool is_selected);
    void draw_feedback(const String& text);
}