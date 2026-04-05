#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"

typedef enum {
    BUTTON_OK = 0,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_COUNT
} button_id_t;

void button_init(uint button_gpio);
bool button_pressed(uint button_gpio);

bool button_was_pressed_event(uint button_gpio);

#endif