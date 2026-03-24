#include <stdbool.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "ntp.h"

extern ssd1306_t disp;

void oled_init_display(ssd1306_t *disp, i2c_inst_t *i2c) {
    disp->external_vcc = false;
    ssd1306_init(disp, 128, 64, 0x3C, i2c);
    ssd1306_clear(disp);
}

void draw_wifi_icon(ssd1306_t *disp, uint32_t pos_x, uint32_t pos_y, bool connected) {

    ////// Draw a simple WiFi icon using pixels //////
    ssd1306_draw_pixel(disp, pos_x + 4, pos_y + 8);
    ssd1306_draw_pixel(disp, pos_x + 4, pos_y + 7);
    ssd1306_draw_pixel(disp, pos_x + 3, pos_y + 7);
    ssd1306_draw_pixel(disp, pos_x + 4, pos_y + 6);
    ssd1306_draw_pixel(disp, pos_x + 5, pos_y + 7);

    ssd1306_draw_pixel(disp, pos_x + 2, pos_y + 5);
    ssd1306_draw_pixel(disp, pos_x + 3, pos_y + 4);
    ssd1306_draw_pixel(disp, pos_x + 4, pos_y + 4);
    ssd1306_draw_pixel(disp, pos_x + 5, pos_y + 4);
    ssd1306_draw_pixel(disp, pos_x + 6, pos_y + 5);

    ssd1306_draw_pixel(disp, pos_x, pos_y + 3);
    ssd1306_draw_pixel(disp, pos_x + 1, pos_y + 2);
    ssd1306_draw_pixel(disp, pos_x + 2, pos_y + 1);
    ssd1306_draw_pixel(disp, pos_x + 3, pos_y + 1);
    ssd1306_draw_pixel(disp, pos_x + 4, pos_y + 1);
    ssd1306_draw_pixel(disp, pos_x + 5, pos_y + 1);
    ssd1306_draw_pixel(disp, pos_x + 6, pos_y + 1);
    ssd1306_draw_pixel(disp, pos_x + 7, pos_y + 2);
    ssd1306_draw_pixel(disp, pos_x + 8, pos_y + 3);
    ////// Draw a simple WiFi icon using pixels //////

    if (!connected) {
        ssd1306_draw_line(disp, pos_x, pos_y, pos_x + 8, pos_y + 8);
    }
}

void draw_status_bar(ssd1306_t *disp, bool wifi_connected, bool gas_ok, bool temp_ok) {
    char time_str[32];
    ntp_get_time_str(time_str, sizeof(time_str));
    ssd1306_draw_line(disp, 0, 0, 127, 0);
    ssd1306_draw_line(disp, 127, 0, 127, 63);
    ssd1306_draw_line(disp, 0, 63, 127, 63);
    ssd1306_draw_line(disp, 0, 0, 0, 63);

    ssd1306_draw_line(disp, 0, 11, 127, 11);
    draw_wifi_icon(disp, 117, 1, wifi_connected);

    if (gas_ok) {
        ssd1306_draw_string(disp, 108, 2, 1, "G");
    }
    if (temp_ok) {
        ssd1306_draw_string(disp, 99, 2, 1, "T");
    }
    ssd1306_draw_string(disp, 2, 2, 1, time_str);
    
}