#ifndef OLED_H
#define OLED_H

#include <stdbool.h>
#include <stdint.h>
#include "ssd1306.h"

typedef struct {
    bool wifi_ok;
    bool gas_ok;
    bool dht_ok;
} oled_flags_t;

typedef struct {
    int t;
    int h;
    uint16_t co_raw;
    uint16_t nh3_raw;
    uint16_t no2_raw;
} sensor_data_t;

typedef enum {
    SCREEN_MAIN = 0,
    SCREEN_SECOND,
    SCREEN_THIRD,
    SCREEN_FOURTH,
    SCREEN_FIFTH,
    SCREEN_COUNT
} oled_screen_t;

// init
void oled_init_display(ssd1306_t *disp, i2c_inst_t *i2c);

// UI normal
void draw_wifi_icon(ssd1306_t *disp, uint32_t x, uint32_t y, bool connected);
void draw_status_bar(ssd1306_t *disp, bool wifi, bool gas_ok, bool temp_ok);

// Screens
void draw_screen_main(ssd1306_t *disp, const oled_flags_t *flags, const sensor_data_t *data);
void draw_screen_second(ssd1306_t *disp, const oled_flags_t *flags, const sensor_data_t *data);
void draw_screen_third(ssd1306_t *disp, const oled_flags_t *flags, const sensor_data_t *data);
void draw_screen_fourth(ssd1306_t *disp, const oled_flags_t *flags, const sensor_data_t *data);
void draw_screen_fifth(ssd1306_t *disp, const oled_flags_t *flags, const sensor_data_t *data);
void draw_current_screen(ssd1306_t *disp,
                        oled_screen_t screen,
                        const oled_flags_t *flags,
                        const sensor_data_t *data);
void draw_icon_1bit(ssd1306_t *disp,
                    uint32_t x,
                    uint32_t y,
                    const uint8_t *icon,
                    uint32_t width,
                    uint32_t height);

// DEBUG
void oled_debug_mark(const char *msg);
void oled_debug_mark2(const char *msg1, const char *msg2);

#endif