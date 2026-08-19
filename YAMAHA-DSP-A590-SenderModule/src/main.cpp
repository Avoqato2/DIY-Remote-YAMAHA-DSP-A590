#include <Arduino.h>
#include <SPI.h>              // Lib for data pins setup
#include <Adafruit_GFX.h>     // Fonts and drawing lib
#include <Adafruit_ST7789.h>  // Hardware-specific library for ST7789
#include <Fonts/FreeSerif12pt7b.h>  // TypewriterFont

// --------------------------------------------------------------------------------
// ---Define Pins---
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

Adafruit_ST7789 tft = Adafruit_ST7789(display_CS, display_DC, -1);


void setup() {
  // Communication speedbetween computer and arduino
  Serial.begin(115200);

  // Setup for Backlight and turn it on
  pinMode(display_BKL, OUTPUT);
  digitalWrite(display_BKL, HIGH);

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
  tft.setFont(&FreeSerif12pt7b);
  tft.setCursor(40, 120);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.print("System is booting....");
  delay(50);

}

void loop() {
  // put your main code here, to run repeatedly:
}