#pragma once
#include <Arduino.h>

namespace WebInterface {

    void init();
    void update();

    void wifi_lost();
    void wifi_connected();

}