#pragma once

#include <string>

struct turn {
    int pin;
    bool toggle;

    turn(int pin, bool toggle)
        : pin(pin), toggle(toggle) {}
};
