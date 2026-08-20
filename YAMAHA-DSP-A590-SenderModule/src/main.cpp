#include <Arduino.h>
#include <SPI.h>                    // Lib for data pins setup
#include <Adafruit_GFX.h>           // Fonts and drawing lib
#include <Adafruit_ST7789.h>        // Hardware-specific library for ST7789
#include <map>                      // For mapping IR codes to strings
#include <Ticker.h>                 // For encoder Guard
#include <WiFi.h>
#include <esp_now.h>                // ESP to ESP Kommunikation

Ticker encoderTicker; // Unser Timer für den Encoder

// --------------------------------------------------------------------------------
// ------------------------------Define Pins---------------------------------------
// --------------------------------------------------------------------------------
// TFT display Pins
#define display_CS  D1  // Chip select line for TFT display
#define display_DC  D2  // Data/command line for TFT display
#define display_BKL D3  // Backlight
#define display_SAD D8  // Serial data line for TFT display
#define display_SCL D9  // Serial clock line for TFT display
// --------------------------------------------------------------------------------
// Rotary encoder Pins
#define rotary_CLK  D4  // Rotary encoder CLK pin
#define rotary_DT   D5  // Rotary encoder DT pin
#define rotary_SW   D0  // Rotary encoder SW pin
// --------------------------------------------------------------------------------
// Button Pins
#define BTN_POWER   D6  // Standby ON/OFF
#define BTN_UP      D7  // Menu Up
#define BTN_DOWN    D10 // Menu Down
// --------------------------------------------------------------------------------
// ------------------------------Define Pins---------------------------------------
// --------------------------------------------------------------------------------

//unsigned long letzteAktivitaet = 0; // for later use, probaly for sleepmode

//----------------------------------------------------------------
//-----------------------State bools------------------------------
//----------------------------------------------------------------
bool is_in_startmenu = true;
bool is_in_channelmenu = false;
bool is_in_effectmenu = false;
bool is_in_test_delay_center_rearmenu = false;
bool aranage_settings_with_encoder = false;
//----------------------------------------------------------------
//-----------------------State bools------------------------------
//----------------------------------------------------------------


// --------------------------------------------------------------------------------
// ------------------------------ESP NOW Section-----------------------------------
// --------------------------------------------------------------------------------
uint8_t basemodule_mac[] = {0xC8, 0xC9, 0xA3, 0x25, 0x3F, 0x02};

typedef struct struct_message {
  char command[32]; 
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

void send_command(String cmd) {
  strncpy(myData.command, cmd.c_str(), sizeof(myData.command));
  esp_err_t result = esp_now_send(basemodule_mac, (uint8_t *) &myData, sizeof(myData));
  
  if (result == ESP_OK) {
    Serial.println("Gesendet: " + cmd);
  } else {
    Serial.println("Fehler beim Senden!");
  }
}

// --------------------------------------------------------------------------------
// ------------------------------ESP NOW Section-----------------------------------
// --------------------------------------------------------------------------------


//----------------------------------------------------------------
//------------------Rotary Encoder Section------------------------
//----------------------------------------------------------------
volatile bool turning_right = false;
volatile bool turning_left = false;


void leseEncoder() {
  static uint8_t old_AB = 3; 
  static int8_t encval = 0;
  static const int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  
  old_AB <<= 2;
  if (digitalRead(rotary_CLK)) old_AB |= 0x02;
  if (digitalRead(rotary_DT))  old_AB |= 0x01;
  
  encval += enc_states[(old_AB & 0x0f)];
  
  if (encval > 3) { 
    turning_right = true;
    encval = 0;
  } else if (encval < -3) { 
    turning_left = true;
    encval = 0;
  }
}

//----------------------------------------------------------------
//------------------Rotary Encoder Section------------------------
//----------------------------------------------------------------


//----------------------------------------------------------------
//-----------------------Menu Section-----------------------------
//----------------------------------------------------------------

Adafruit_ST7789 tft = Adafruit_ST7789(display_CS, display_DC, -1); // Create an instance of the display driver

//-----------------------Menu variables----------------------------
int current_selected_item = 0;  // Index of the currently selected item in the menu
int last_selected_item = -1;    // Index of the last selected item, used to detect changes

int menu_offset = 0;            // Index of the item at the top of the screen, for scrolling
const int MAX_VISIBLE = 6;      // how many items can be displayed on the screen at once

//-------------Channel Menu and Effect Menu Stringsarrays----------------
const int NUM_CHANNELS = 8;
String channel_str_arr[NUM_CHANNELS] = {
  "Back","LD/TV", "CD", "PHONO", "VIDEO AUX", "TUNER", "VCR1", "VCR2"
};

String channleCMD_str_arr[NUM_CHANNELS] = {
  "DUMMY", "ld/tv", "cd", "phono", "video_aux", "tuner", "vcr1", "vrc2"
};

const int NUM_EFFECTS = 11;
String effect_str_arr[NUM_EFFECTS] = {
  "Back","Effect ON/OFF", "SETTINGS","PROLOGIC", "ENHANCED", "CONCERT HALL", "CONCERT VIDEO",
  "ROCK CONCERT", "DISCO", "MONO MOVIE", "STADIUM"
};

String effectCMD_str_arr[NUM_EFFECTS] = {
  "DUMMY", "effect_on_off", "DUMMY", "prologic", "enhanced", "concert_hall", "concert_video", 
  "rock_concert", "disco", "mono_movie", "stadium"
};

const int NUM_TEST_DELAY_CENTER_REAR = 5;
String test_delay_center_rear_str_arr[NUM_TEST_DELAY_CENTER_REAR] = {
  "Back", "TEST", "DELAY", "CENTER", "REAR"
};

//
// EFFECT SETTING CMD IMPLAMATATION
//

const int NUM_STARTMENU = 2;
String startmenu_str_arr[NUM_STARTMENU] = {
  "CHANNELS", "EFFECTS"
};

void screen_boot(){
  // Setup for SPI
  // SCL Tikrate, -1 not used, SAD data, CS chip select.
  SPI.begin(display_SCL, -1, display_SAD, display_CS);
  // Initiate Display with pixel count
  tft.init(240, 280);
  // Turn screen so the long side is the bottom.
  tft.setRotation(1);
  // Set Screen to full Black so any Pixelerrors are gone
  tft.fillScreen(ST77XX_BLACK);
  // System Message
  tft.setCursor(25, 100);
  tft.setTextColor(ST77XX_ORANGE);
  tft.setTextSize(2);
  tft.print("System is booting...");
  delay(2000);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 100);
  tft.print("YAMAHA DSP A590 REMOTE");
  delay(2000);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(25, 100);
  tft.print("Made by: @avoqato2");
  delay(2000);
}

void draw_header(){
  // just a simple hader for the menu
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 20);
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);
  tft.print("Yamah DSP A590 REMOTE");
  tft.drawLine(0, 40, 280, 40, ST77XX_RED);
}
void draw_menu(String (menu[]), int NUM_ITEMS){
  draw_header();

  int startY = 60; // good space between header and first item
  int itemHight = 30; // gab between items

  for(int i = 0; i < MAX_VISIBLE; i++){
    int item_index = menu_offset + i; // Calculate the index of the item to display based on the current offset
    // set cursor position for items
    tft.setCursor(10, startY + (i * itemHight));
    if (item_index < NUM_ITEMS) { // Check if the item index is within the bounds of the menu array
      if(item_index == current_selected_item){
        tft.setTextColor(ST77XX_BLACK, ST77XX_ORANGE); // Highlight selected item
        tft.print("> " + String(menu[item_index]) + "      ");
      } else {
        tft.setTextColor(ST77XX_ORANGE, ST77XX_BLACK); // Normal color for other items
        tft.print("  " + String(menu[item_index]) + "      ");
      }
    }else {
      // Wenn das Menü zu Ende ist, überschreiben wir alte Einträge mit Leerzeichen
      tft.setTextColor(ST77XX_ORANGE, ST77XX_BLACK); 
      tft.print("                        "); 
    }
  }
}

void draw_feedback(String text) {
  // draw what u selectet
  tft.fillRect(0, 210, 280, 30, ST77XX_BLUE);
  tft.setCursor(50, 218);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.print(text);
  
  // delete what have been written
  delay(800);
  tft.fillRect(0, 210, 280, 30, ST77XX_BLACK);
  
  // Force to draw the menu again
  last_selected_item = -1; 
}

void startmenu_selection_cases(){
   if(is_in_startmenu){ // If we are in the start menu, check which item is selected and navigate accordingly
        if(current_selected_item == 0){
          is_in_startmenu = false;
          is_in_channelmenu = true;
          current_selected_item = 0;
          last_selected_item = 0;
          menu_offset = 0;
          draw_menu(channel_str_arr, NUM_CHANNELS);
        } else if(current_selected_item == 1){
          is_in_startmenu = false;
          is_in_effectmenu = true;
          current_selected_item = 0;
          last_selected_item = 0;
          menu_offset = 0;
          draw_menu(effect_str_arr, NUM_EFFECTS);
        }
      }else if(is_in_channelmenu || is_in_effectmenu && !is_in_test_delay_center_rearmenu){ // If we are in the channel or effect menu, check if the "Back" item is selected to return to the start menu
        if(current_selected_item == 0){
          is_in_startmenu = true;
          is_in_channelmenu = false;
          is_in_effectmenu = false;
          current_selected_item = 0;
          last_selected_item = 0;
          menu_offset = 0;
          draw_menu(startmenu_str_arr, NUM_STARTMENU);
        }
      }
}

void effectmenu_selection_cases(){
  if(is_in_effectmenu && !is_in_test_delay_center_rearmenu){
        if(current_selected_item == 2){
          is_in_test_delay_center_rearmenu = true;
          current_selected_item = 0;
          last_selected_item = 0;
          menu_offset = 0;
          draw_menu(test_delay_center_rear_str_arr, NUM_TEST_DELAY_CENTER_REAR);
        }
      }else if(is_in_test_delay_center_rearmenu){
        if(current_selected_item == 0){
          is_in_test_delay_center_rearmenu = false;
          current_selected_item = 0;
          last_selected_item = 0;
          menu_offset = 0;
          draw_menu(effect_str_arr, NUM_EFFECTS);
        }else if (current_selected_item == 1){
          //
          // TEST BUTTON IMPLEMETATION
          //
        }else if(current_selected_item == 2 || current_selected_item == 3 || current_selected_item == 4){
          aranage_settings_with_encoder = true;
          //
          // Arange Delay/Center/Rear IMPLAMANTATION
          //
        }
      }
}

void handle_menu_navigation(String menu[], int NUM_ITEMS){

  if (digitalRead(rotary_SW) == LOW && !turning_left && !turning_right) {
      // letzteAktivitaet = millis(); // for later use, probaly for sleepmode
      startmenu_selection_cases();
      effectmenu_selection_cases();
      if(current_selected_item != 0 && !is_in_startmenu){

        // Very UGLY IMPLAMANTATION
        if(is_in_channelmenu){
          send_command(channleCMD_str_arr[current_selected_item]);
        }else if(is_in_effectmenu){
          send_command(effectCMD_str_arr[current_selected_item]);
        }
        draw_feedback(" Transmit: " + menu[current_selected_item]);
      }
      while(digitalRead(rotary_SW) == LOW) { delay(10); } 
    }

  if (digitalRead(BTN_UP) == LOW) {
      // letzteAktivitaet = millis(); // for later use, probaly for sleepmode
      current_selected_item--;
      if (current_selected_item < 0) current_selected_item = NUM_ITEMS -1;
      while(digitalRead(BTN_UP) == LOW) { delay(10); }
    }

    if (digitalRead(BTN_DOWN) == LOW) {
      // letzteAktivitaet = millis(); // for later use, probaly for sleepmode
      current_selected_item++;
      if (current_selected_item >= NUM_ITEMS) current_selected_item = 0; 
      while(digitalRead(BTN_DOWN) == LOW) { delay(10); }
    }

    if (current_selected_item != last_selected_item) {
      if (current_selected_item < menu_offset) { // If the selected item is above the visible range, scroll up
        menu_offset = current_selected_item; 
      } else if (current_selected_item >= menu_offset + MAX_VISIBLE ) { // If the selected item is below the visible range, scroll down
        menu_offset = current_selected_item - MAX_VISIBLE + 1;
      }
      draw_menu(menu, NUM_ITEMS);
      last_selected_item = current_selected_item;
  }
}

//----------------------------------------------------------------
//-----------------------Menu Section-----------------------------
//----------------------------------------------------------------

void setup() {
  // Communication speedbetween computer and arduino
  Serial.begin(115200);
   // Setup for Backlight and turn it on
  pinMode(display_BKL, OUTPUT);
  digitalWrite(display_BKL, HIGH);
  //Button pins setup
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_POWER, INPUT_PULLUP);
  //Rotary pins setup
  pinMode(rotary_SW, INPUT_PULLUP);
  pinMode(rotary_CLK, INPUT_PULLUP);
  pinMode(rotary_DT, INPUT_PULLUP);

  // WLAN & ESP-NOW starten
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Fehler bei ESP-NOW");
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(10, 40);
    tft.print("Funk-Fehler!");
    return; // Abbruch bei Fehler
  }

  // Empfänger hinzufügen
  memcpy(peerInfo.peer_addr, basemodule_mac, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Fehler Empfaenger");
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(10, 40);
    tft.print("Empfaenger fehlt!");
    return;
  }


  //Ecoder Ticker = some kinde of guard for noisdefelction
  encoderTicker.attach_ms(4, leseEncoder);
  //Boot Screen
  screen_boot();
}

void loop() {

  if(digitalRead(BTN_POWER)== LOW){
    send_command("standby");
    draw_feedback(" Transmit: Standby");
  }

  // it may be better to 
  // count how many steps 
  // you turn the Encoder 
  // and than send the steps
  // over to the Base Module
  if(turning_right == true){
    //
    // VOLUME UP IMPLIMANTATION
    //
    turning_right = false;
    send_command("volume_up");
    draw_feedback(" Transmit: Volume UP");
  }

  if(turning_left == true){
    //
    // VOLUME UP IMPLIMANTATION
    //
    turning_left = false;
    send_command("volume_down");
    draw_feedback(" Transmit: Volume DOWN");
  }


  //Startmenu State
  if (is_in_startmenu) {
    handle_menu_navigation(startmenu_str_arr, NUM_STARTMENU);
  }

  //Channle State
  if(is_in_channelmenu) {
    handle_menu_navigation(channel_str_arr, NUM_CHANNELS);
  }

  //Effect state
  if(is_in_effectmenu) {
    if(is_in_test_delay_center_rearmenu){
        handle_menu_navigation(test_delay_center_rear_str_arr, NUM_TEST_DELAY_CENTER_REAR);
    } else {
      handle_menu_navigation(effect_str_arr, NUM_EFFECTS);
    }
  }

  delay(10);
}