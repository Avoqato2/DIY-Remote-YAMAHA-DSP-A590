#include <Arduino.h>
#include <WiFi.h>
#include "driver/gpio.h"

#include "../include/config.h"
#include "../lib/Display/Display_Driver.h"
#include "../lib/Input/Input.h"
#include "../lib/Menu/Menu.h"
#include "../lib/Network/Network.h"

unsigned long last_activity = 0;

void do_deepsleep() {
    WiFi.mode(WIFI_OFF); 
    Display::sleep(); 

    esp_deep_sleep_enable_gpio_wakeup(1ULL << Config::PIN_ENC_SW, ESP_GPIO_WAKEUP_GPIO_LOW);
    esp_deep_sleep_start();       
}

void setup() {
    Serial.begin(115200);
    
    gpio_hold_dis((gpio_num_t)Config::PIN_TFT_BKL);
    pinMode(Config::PIN_TFT_BKL, OUTPUT);
    digitalWrite(Config::PIN_TFT_BKL, HIGH);

    bool cold_boot = (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED);

    Display::init(cold_boot);
    Input::init();
    Network::init();
    Menu::init();

    last_activity = millis();
}

void loop() {
    // 1. Taster entprellen und Hardware auslesen (asynchron)
    Input::update();

    // 2. Prüfen, ob wir die 20-Sekunden Uhr zurücksetzen müssen
    if (Input::has_activity()) {
        last_activity = millis();
    }

    // 3. Standby check
    if (Input::is_power_pressed()) {
        Network::send_command("standby");
        Display::draw_feedback(" Transmit: Standby");
    }

    // 4. Menü & Lautstärke Warteschlange abarbeiten
    Menu::update();

    // 5. Blauen Balken aufräumen
    Display::update();

    // 6. Tiefschlaf
    if (millis() - last_activity > Config::TIMEOUT_MS) {
        do_deepsleep();
    }
    
    // KEIN delay(10) MEHR HIER!
}