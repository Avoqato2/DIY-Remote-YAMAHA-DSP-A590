#include "Input.h"
#include "../../include/config.h"
#include <Ticker.h>

namespace Input {
    Ticker encoderTicker;
    volatile int times_turn_right = 0;
    volatile int times_turn_left = 0;
    bool activity_flag = false;

    // Struktur für asynchrones Entprellen (Debouncing)
    struct Button {
        uint8_t pin;
        bool state;
        bool last_state;
        unsigned long last_debounce_time;
        bool pressed_flag;
    };

    Button btn_up = {Config::PIN_BTN_UP, HIGH, HIGH, 0, false};
    Button btn_down = {Config::PIN_BTN_DOWN, HIGH, HIGH, 0, false};
    Button btn_power = {Config::PIN_BTN_POWER, HIGH, HIGH, 0, false};
    Button btn_sw = {Config::PIN_ENC_SW, HIGH, HIGH, 0, false};

    const unsigned long DEBOUNCE_DELAY = 50;

    void IRAM_ATTR leseEncoder() {
        static uint8_t old_AB = 3; 
        static int8_t encval = 0;
        static const int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
        
        old_AB <<= 2;
        if (digitalRead(Config::PIN_ENC_CLK)) old_AB |= 0x02;
        if (digitalRead(Config::PIN_ENC_DT))  old_AB |= 0x01;
        
        encval += enc_states[(old_AB & 0x0f)];
        
        if (encval > 3) { 
            times_turn_right += 2;
            encval = 0;
            activity_flag = true;
        } else if (encval < -3) { 
            times_turn_left += 2;
            encval = 0;
            activity_flag = true;
        }
    }

    void init() {
        pinMode(Config::PIN_BTN_UP, INPUT); // Ggf. INPUT_PULLUP, falls keine externen Widerstände verbaut sind
        pinMode(Config::PIN_BTN_DOWN, INPUT);
        pinMode(Config::PIN_BTN_POWER, INPUT);
        pinMode(Config::PIN_ENC_SW, INPUT);
        pinMode(Config::PIN_ENC_CLK, INPUT);
        pinMode(Config::PIN_ENC_DT, INPUT);

        encoderTicker.attach_ms(4, leseEncoder);
    }

    void update_button(Button& btn) {
        bool reading = digitalRead(btn.pin);
        
        if (reading != btn.last_state) {
            btn.last_debounce_time = millis();
        }
        
        if ((millis() - btn.last_debounce_time) > DEBOUNCE_DELAY) {
            if (reading != btn.state) {
                btn.state = reading;
                // LOW bedeutet Knopf gedrückt (bei Pullup)
                if (btn.state == LOW) {
                    btn.pressed_flag = true;
                    activity_flag = true;
                }
            }
        }
        btn.last_state = reading;
    }

    void update() {
        update_button(btn_up);
        update_button(btn_down);
        update_button(btn_power);
        update_button(btn_sw);
    }

    int get_and_clear_right_turns() {
        noInterrupts();
        int steps = times_turn_right;
        times_turn_right = 0;
        interrupts();
        return steps;
    }

    int get_and_clear_left_turns() {
        noInterrupts();
        int steps = times_turn_left;
        times_turn_left = 0;
        interrupts();
        return steps;
    }

    // Flags abholen und sofort wieder löschen
    bool check_flag(Button& btn) {
        if (btn.pressed_flag) {
            btn.pressed_flag = false;
            return true;
        }
        return false;
    }

    bool is_up_pressed() { return check_flag(btn_up); }
    bool is_down_pressed() { return check_flag(btn_down); }
    bool is_sw_pressed() { return check_flag(btn_sw); }
    bool is_power_pressed() { return check_flag(btn_power); }
    
    bool has_activity() {
        if(activity_flag) {
            activity_flag = false;
            return true;
        }
        return false;
    }
}