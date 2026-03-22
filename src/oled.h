#ifndef OLED_H
#define OLED_H

#include <stdbool.h>
#include <stdint.h>
#include "ssd1306.h"

// init
void oled_init_display(ssd1306_t *disp, i2c_inst_t *i2c);

// UI normal
void draw_wifi_icon(ssd1306_t *disp, uint32_t x, uint32_t y, bool connected);
void draw_status_bar(ssd1306_t *disp, bool wifi, bool gas_ok, bool temp_ok);

// DEBUG
void oled_debug_mark(const char *msg);
void oled_debug_mark2(const char *msg1, const char *msg2);

#endif