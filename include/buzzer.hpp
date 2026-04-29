#pragma once

#include <esp_err.h>


class Buzzer {
    public:
        esp_err_t setup();
        esp_err_t playLimitBeep();
};
