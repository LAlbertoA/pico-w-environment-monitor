#include "buttons.h"
#include "pico/stdlib.h"

void button_init(uint button_gpio) {
    gpio_init(button_gpio);
    gpio_set_dir(button_gpio, GPIO_IN);
    gpio_pull_up(button_gpio);
}

bool button_pressed(uint button_gpio) {
    return gpio_get(button_gpio) == 0;
}

bool button_was_pressed_event(uint button_gpio) {
    static bool last_raw_state = false;
    static bool stable_state = false;
    static uint32_t last_change_ms = 0;

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());

    bool raw_state = (gpio_get(button_gpio) == 0);

    if (raw_state != last_raw_state) {
        last_raw_state = raw_state;
        last_change_ms = now_ms;
    }

    if ((now_ms - last_change_ms) >= 20) {

        if (stable_state != raw_state) {
            stable_state = raw_state;

            if (stable_state) {
                return true;
            }
        }
    }

    return false;
}