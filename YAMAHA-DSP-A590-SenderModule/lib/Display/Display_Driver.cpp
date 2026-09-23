#include "Display_Driver.h"
#include "../../include/config.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "driver/gpio.h" // Nötig für gpio_hold_en

Adafruit_ST7789 tft(Config::PIN_TFT_CS, Config::PIN_TFT_DC, -1);

namespace Display {
    unsigned long feedback_timer = 0;
    bool feedback_active = false;

    void safe_delay(unsigned long ms) {
        unsigned long start = millis();
        while (millis() - start < ms) { yield(); }
    }

    void show_boot_screen() {
        tft.setCursor(25, 100);
        tft.setTextColor(ST77XX_ORANGE);
        tft.setTextSize(2);
        tft.print("System is booting...");
        safe_delay(2000);
        tft.fillScreen(ST77XX_BLACK);
        tft.setCursor(10, 100);
        tft.print("YAMAHA DSP A590 REMOTE");
        safe_delay(2000);
        tft.fillScreen(ST77XX_BLACK);
        tft.setCursor(25, 100);
        tft.print("Made by: @avoqato2");
        safe_delay(2000);
    }

    void init(bool cold_boot) {
        SPI.begin(Config::PIN_TFT_SCL, -1, Config::PIN_TFT_SAD, Config::PIN_TFT_CS);
        tft.init(240, 280);
        tft.setRotation(1);
        tft.fillScreen(ST77XX_BLACK);

        if(cold_boot){
            show_boot_screen();
        }
    }

    void update() {
        if (feedback_active && (millis() - feedback_timer >= 1000)) {
            tft.fillRect(0, 210, 280, 30, ST77XX_BLACK);
            feedback_active = false;
        }
    }

    void sleep() {
        tft.fillScreen(ST77XX_BLACK);
        digitalWrite(Config::PIN_TFT_BKL, LOW);   
        gpio_hold_en((gpio_num_t)Config::PIN_TFT_BKL);
        gpio_deep_sleep_hold_en();
        tft.enableSleep(true); 
    }

    void draw_header() {
        tft.fillRect(0, 0, 280, 210, ST77XX_BLACK);
        tft.setCursor(10, 20);
        tft.setTextColor(ST77XX_RED);
        tft.setTextSize(2);
        tft.print("YAMAHA DSP A590 REMOTE");
        tft.drawLine(0, 40, 280, 40, ST77XX_RED);
    }

    void draw_menu_item(int y, const String& text, bool is_selected) {
        tft.setCursor(10, y);
        if (is_selected) {
            tft.setTextColor(ST77XX_BLACK, ST77XX_ORANGE);
            tft.print("> " + text + "     ");
        } else {
            tft.setTextColor(ST77XX_ORANGE, ST77XX_BLACK);
            tft.print("  " + text);
        }
    }

    void draw_feedback(const String& text) {
        tft.fillRect(0, 210, 280, 30, ST77XX_BLUE);
        tft.setCursor(50, 218);
        tft.setTextColor(ST77XX_WHITE);
        tft.setTextSize(1);
        tft.print(text);
        feedback_timer = millis();
        feedback_active = true;
    }
}