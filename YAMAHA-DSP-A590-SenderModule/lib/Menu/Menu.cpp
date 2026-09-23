#include "Menu.h"
#include "../Display/Display_Driver.h"
#include "../Input/Input.h"
#include "../Network/Network.h"

namespace Display {
    void trigger_menu_redraw(); 
}

namespace Menu {
    RTC_DATA_ATTR bool is_in_startmenu = true;
    RTC_DATA_ATTR bool is_in_channelmenu = false;
    RTC_DATA_ATTR bool is_in_effectmenu = false;
    RTC_DATA_ATTR bool is_in_test_delay_center_rearmenu = false;
    RTC_DATA_ATTR bool effencts_are_on = false;
    RTC_DATA_ATTR bool aranage_settings_with_encoder = false;

    int current_selected_item = 0; 
    int last_selected_item = -1;    
    int menu_offset = 0;            
    const int MAX_VISIBLE = 5;      

    // --- Asynchrone Warteschlange für Lautstärke ---
    int pending_volume_steps = 0;
    String pending_volume_cmd = "";
    unsigned long last_volume_send = 0;

    const int NUM_CHANNELS = 8;
    String channel_str_arr[NUM_CHANNELS] = {"Back","LD/TV", "CD", "PHONO", "VIDEO AUX", "TUNER", "VCR1", "VCR2"};
    String channleCMD_str_arr[NUM_CHANNELS] = {"DUMMY", "ld/tv", "cd", "phono", "video_aux", "tuner", "vcr1", "vcr2"};

    const int NUM_EFFECTS = 11;
    String effect_str_arr[NUM_EFFECTS] = {"Back","Effect ON/OFF", "SETTINGS","PROLOGIC", "ENHANCED", "CONCERT HALL", "CONCERT VIDEO", "ROCK CONCERT", "DISCO", "MONO MOVIE", "STADIUM"};
    String effectCMD_str_arr[NUM_EFFECTS] = {"DUMMY", "effect_on_off", "DUMMY", "prologic", "enhanced", "concert_hall", "concert_video", "rock_concert", "disco", "mono_movie", "stadium"};

    const int NUM_TEST_DELAY = 5;
    String test_delay_str_arr[NUM_TEST_DELAY] = {"Back", "TEST", "DELAY", "CENTER", "REAR"};

    const int NUM_STARTMENU = 2;
    String startmenu_str_arr[NUM_STARTMENU] = {"CHANNELS", "EFFECTS"};

    void draw_active_menu() {
        const String* active_arr = nullptr;
        int active_size = 0;

        if(is_in_startmenu) { active_arr = startmenu_str_arr; active_size = NUM_STARTMENU; }
        else if(is_in_channelmenu) { active_arr = channel_str_arr; active_size = NUM_CHANNELS; }
        else if(is_in_effectmenu) { active_arr = effect_str_arr; active_size = NUM_EFFECTS; }
        else if(is_in_test_delay_center_rearmenu) { active_arr = test_delay_str_arr; active_size = NUM_TEST_DELAY; }

        if(!active_arr) return;

        Display::draw_header();
        int startY = 60; 
        int itemHeight = 30; 

        if (current_selected_item >= menu_offset + MAX_VISIBLE) menu_offset = current_selected_item - MAX_VISIBLE + 1;
        else if (current_selected_item < menu_offset) menu_offset = current_selected_item;

        for(int i = 0; i < MAX_VISIBLE; i++){
            int item_index = menu_offset + i; 
            int yPos = startY + (i * itemHeight);
            
            if (item_index < active_size) {
                Display::draw_menu_item(yPos, active_arr[item_index], (item_index == current_selected_item));
            } else {
                Display::draw_menu_item(yPos, "        ", false); 
            }
        }
    }

    void init() { draw_active_menu(); }

    void fire_volume_logic(int steps, const String& transmit_str, const String& volume_cmd) {
        if(steps > 0) {
            Display::draw_feedback(transmit_str);
            pending_volume_steps += steps; // In die Warteschlange packen
            pending_volume_cmd = volume_cmd;
        }
    }

    void handle_encoder_turns() {
        int right_steps = Input::get_and_clear_right_turns();
        int left_steps = Input::get_and_clear_left_turns();

        if (right_steps == 0 && left_steps == 0) return;

        if (!aranage_settings_with_encoder) {
            if (right_steps > 0) fire_volume_logic(right_steps, " Transmit: Volume UP", "volume_up");
            if (left_steps > 0) fire_volume_logic(left_steps, " Transmit: Volume DOWN", "volume_down");
        } else {
            if (current_selected_item == 2) {
                if (right_steps > 0) fire_volume_logic(right_steps, " Transmit: Delay +", "delay_up");
                if (left_steps > 0) fire_volume_logic(left_steps, " Transmit: Delay -", "delay_down");
            } else if (current_selected_item == 3) {
                if (right_steps > 0) fire_volume_logic(right_steps, " Transmit: Center +", "center_up");
                if (left_steps > 0) fire_volume_logic(left_steps, " Transmit: Center -", "center_down");
            } else if (current_selected_item == 4) {
                if (right_steps > 0) fire_volume_logic(right_steps, " Transmit: Rear +", "rear_up");
                if (left_steps > 0) fire_volume_logic(left_steps, " Transmit: Rear -", "rear_down");
            }
        }
    }

    void update() {
        handle_encoder_turns(); 

        // Hintergrund-Abarbeitung der Lautstärke ohne delay()!
        if (pending_volume_steps > 0) {
            if (millis() - last_volume_send > 40) {
                Network::send_command(pending_volume_cmd);
                pending_volume_steps--;
                last_volume_send = millis();
            }
        }

        int active_size = is_in_startmenu ? NUM_STARTMENU : 
                          is_in_channelmenu ? NUM_CHANNELS : 
                          is_in_effectmenu ? NUM_EFFECTS : NUM_TEST_DELAY;

        if (Input::is_up_pressed()) {
            current_selected_item--;
            if (current_selected_item < 0) current_selected_item = active_size - 1;
        }

        if (Input::is_down_pressed()) {
            current_selected_item++;
            if (current_selected_item >= active_size) current_selected_item = 0;
        }

        if (Input::is_sw_pressed()) {
            if (is_in_startmenu) {
                is_in_startmenu = false;
                if(current_selected_item == 0) is_in_channelmenu = true;
                else if(current_selected_item == 1) is_in_effectmenu = true;
                current_selected_item = 0; menu_offset = 0;
            } 
            else if (is_in_channelmenu || (is_in_effectmenu && !is_in_test_delay_center_rearmenu)) {
                if (current_selected_item == 0) {
                    is_in_startmenu = true; is_in_channelmenu = false; is_in_effectmenu = false;
                    current_selected_item = 0; menu_offset = 0;
                } else if (is_in_channelmenu) {
                    Network::send_command(channleCMD_str_arr[current_selected_item]);
                    Display::draw_feedback(" Transmit: " + channel_str_arr[current_selected_item]);
                } else if (is_in_effectmenu) {
                    if (current_selected_item == 2 && effencts_are_on) {
                        is_in_test_delay_center_rearmenu = true; is_in_effectmenu = false;
                        current_selected_item = 0; menu_offset = 0;
                    } else if (current_selected_item == 2 && !effencts_are_on) {
                        Display::draw_feedback("Turn Effects ON!");
                    } else {
                        effencts_are_on = (current_selected_item != 1) ? true : !effencts_are_on;
                        Network::send_command(effectCMD_str_arr[current_selected_item]);
                        Display::draw_feedback(" Transmit: " + effect_str_arr[current_selected_item]);
                    }
                }
            } 
            else if (is_in_test_delay_center_rearmenu) {
                if (current_selected_item == 0) {
                    is_in_test_delay_center_rearmenu = false; aranage_settings_with_encoder = false;
                    is_in_effectmenu = true; current_selected_item = 0; menu_offset = 0;
                } else if (current_selected_item == 1) {
                    Network::send_command("test");
                    Display::draw_feedback(" Transmit: Test");
                } else {
                    aranage_settings_with_encoder = !aranage_settings_with_encoder;
                    Display::draw_feedback(" Disabled/Able: " + test_delay_str_arr[current_selected_item]);
                }
            }
            last_selected_item = -1; // Erzwingt Redraw
        }

        if (current_selected_item != last_selected_item) {
            draw_active_menu();
            last_selected_item = current_selected_item;
        }
    }
}