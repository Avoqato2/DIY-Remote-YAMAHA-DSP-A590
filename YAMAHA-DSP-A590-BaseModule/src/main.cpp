#include <Arduino.h>

#include "../lib/IRControl/IRControl.h"
#include "../lib/Network/Network.h"
#include "../lib/WebInterface/WebInterface.h"

void setup()
{
    Serial.begin(115200);

    IRControl::init();
    Network::init();
    WebInterface::init();
}

void loop()
{
    IRControl::update();
    WebInterface::update();
    Network::update();
}