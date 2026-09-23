#pragma once
#include <Arduino.h>

namespace Config {
    // --- TFT Display Pins ---
    constexpr uint8_t PIN_TFT_CS  = D1;  // D1 (Achte hier auf das ESP32 Pin-Mapping!)
    constexpr uint8_t PIN_TFT_DC  = D7; // D7
    constexpr uint8_t PIN_TFT_BKL = D3;  // D3
    constexpr uint8_t PIN_TFT_SAD = D8; // D8
    constexpr uint8_t PIN_TFT_SCL = D10;  // D10

    // --- Rotary Encoder Pins ---
    constexpr uint8_t PIN_ENC_CLK = D4;  // D4
    constexpr uint8_t PIN_ENC_DT  = D5; // D5
    constexpr uint8_t PIN_ENC_SW  = D0; // D0

    // --- Button Pins ---
    constexpr uint8_t PIN_BTN_POWER = D6; // D6
    constexpr uint8_t PIN_BTN_UP    = D2;  // D2
    constexpr uint8_t PIN_BTN_DOWN  = D9;  // D9

    // --- System Settings ---
    constexpr unsigned long TIMEOUT_MS = 20000;
    
    // --- Network ---
    const uint8_t BASE_MAC[] = {0xC8, 0xC9, 0xA3, 0x25, 0x3F, 0x02};
}