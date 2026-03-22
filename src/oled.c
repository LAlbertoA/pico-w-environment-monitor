#include "ssd1306.h"

void oled_init_display(ssd1306_t *disp) {
    disp->external_vcc = false;
    ssd1306_init(disp, 128, 64, 0x3C, I2C_PORT);
    ssd1306_clear(disp);
}

void oled_show_status_bar(ssd1306_t *disp) {

}