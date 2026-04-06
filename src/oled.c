#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "ntp.h"
#include "oled.h"

extern ssd1306_t disp;

// Simple icons for temperature, humidity, and air quality (gas).
static const uint8_t icon_temp[] = {
    0x01, 0x80, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40,
    0x02, 0x40, 0x0a, 0x40, 0x0a, 0x40, 0x0a, 0x40,
    0x0a, 0x40, 0x11, 0x20, 0x21, 0x10, 0x21, 0x10,
    0x21, 0x10, 0x21, 0x10, 0x1f, 0xf0, 0x0e, 0x00
};

static const uint8_t icon_hum[] = {
    0x01, 0x80, 0x01, 0x80, 0x03, 0xc0, 0x03, 0xc0,
    0x07, 0xe0, 0x07, 0xe0, 0x0f, 0xf0, 0x0f, 0xf0,
    0x1f, 0xf8, 0x1f, 0xf8, 0x3f, 0xfc, 0x3f, 0xfc,
    0x3f, 0xfc, 0x1f, 0xf8, 0x0f, 0xf0, 0x03, 0xc0
};

static const uint8_t icon_air[] = {
    0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 0x07, 0xe0,
    0x1c, 0x38, 0x30, 0x0c, 0x30, 0x0c, 0x7f, 0xfe,
    0x7f, 0xfe, 0x60, 0x06, 0x60, 0x06, 0x30, 0x0c,
    0x3f, 0xfc, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t icon_temp_32[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x07, 0xE0, 0x00, 0x00, 0x0C, 0x30, 0x00,
    0x00, 0x0C, 0x30, 0x00, 0x00, 0x0C, 0x30, 0x00,
    0x00, 0x0C, 0x30, 0x00, 0x00, 0x0C, 0x30, 0x00,
    0x00, 0x0C, 0x30, 0x00, 0x00, 0x0C, 0x30, 0x00,
    0x00, 0x0C, 0x30, 0x00, 0x00, 0x0C, 0x30, 0x00,
    0x00, 0x0C, 0x30, 0x00, 0x00, 0x0C, 0x30, 0x00,
    0x00, 0x0D, 0xB0, 0x00, 0x00, 0x0D, 0xB0, 0x00,
    0x00, 0x0D, 0xB0, 0x00, 0x00, 0x0D, 0xB0, 0x00,
    0x00, 0x0D, 0xB0, 0x00, 0x00, 0x0D, 0xB0, 0x00,
    0x00, 0x0D, 0xB0, 0x00, 0x00, 0x0D, 0xB0, 0x00,
    0x00, 0x0F, 0xF0, 0x00, 0x00, 0x1F, 0xF8, 0x00,
    0x00, 0x3F, 0xFC, 0x00, 0x00, 0x7F, 0xFE, 0x00,
    0x00, 0x7F, 0xFE, 0x00, 0x00, 0x7F, 0xFE, 0x00,
    0x00, 0x3F, 0xFC, 0x00, 0x00, 0x1F, 0xF8, 0x00,
    0x00, 0x07, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Function to draw a 1-bit-per-pixel icon on the OLED at a specified position.
// The icon is expected to be a
void draw_icon_1bit(ssd1306_t *disp,
                    uint32_t x,
                    uint32_t y,
                    const uint8_t *icon,
                    uint32_t width,
                    uint32_t height) {
    uint32_t bytes_per_row = (width + 7) / 8;

    for (uint32_t row = 0; row < height; row++) {
        for (uint32_t col = 0; col < width; col++) {
            uint32_t byte_index = row * bytes_per_row + (col / 8);
            uint8_t byte = icon[byte_index];
            uint8_t mask = 0x80 >> (col % 8);
            if (byte & mask) {
                ssd1306_draw_pixel(disp, x + col, y + row);
            }
        }
    }
}

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

void draw_screen_main(ssd1306_t *disp, const oled_flags_t *flags, const sensor_data_t *data) {
    char buf1[32];
    char buf2[32];

    ssd1306_clear(disp);
    draw_status_bar(disp, flags->wifi_ok, flags->gas_ok, flags->dht_ok);

    ssd1306_draw_string(disp, 2, 12, 1, flags->wifi_ok ? "WiFi OK" : "WiFi FAIL");
    
    if (flags->dht_ok) {
        snprintf(buf1, sizeof(buf1), "T:%dC H:%d%%", data->t, data->h);
    } else {
        snprintf(buf1, sizeof(buf1), "DHT11 FAIL");
    }
    ssd1306_draw_string(disp, 2, 22, 1, buf1);

    if (flags->gas_ok) {
        snprintf(buf1, sizeof(buf1), "CO:%u NH3:%u", data->co_raw, data->nh3_raw);
        snprintf(buf2, sizeof(buf2), "NO2:%u", data->no2_raw);
    } else {
        snprintf(buf1, sizeof(buf1), "GAS (CO, NH3) FAIL");
        snprintf(buf2, sizeof(buf2), "GAS (NO2) FAIL");
    }

    ssd1306_draw_string(disp, 2, 32, 1, buf1);
    ssd1306_draw_string(disp, 2, 42, 1, buf2);

    ssd1306_show(disp);
}

void draw_screen_second(ssd1306_t *disp, const oled_flags_t *flags, const sensor_data_t *data) {
    char buf1[32];
    char buf2[32];

    ssd1306_clear(disp);
    draw_status_bar(disp, flags->wifi_ok, flags->gas_ok, flags->dht_ok);

    if (flags->dht_ok) {
        snprintf(buf1, sizeof(buf1), "T:%dC", data->t);
        snprintf(buf2, sizeof(buf2), "H:%d%%", data->h);
    } else {
        snprintf(buf1, sizeof(buf1), "DHT11 FAIL");
        snprintf(buf2, sizeof(buf2), "Waiting ...");
    }
    ssd1306_draw_string(disp, 2, 13, 2, buf1);
    ssd1306_draw_string(disp, 2, 33, 2, buf2);

    ssd1306_show(disp);
}

void draw_screen_third(ssd1306_t *disp, const oled_flags_t *flags, const sensor_data_t *data) {
    char buf1[32];
    char buf2[32];
    char buf3[32];

    ssd1306_clear(disp);
    draw_status_bar(disp, flags->wifi_ok, flags->gas_ok, flags->dht_ok);

    if (flags->gas_ok) {
        snprintf(buf1, sizeof(buf1), "CO:%u", data->co_raw);
        snprintf(buf2, sizeof(buf2), "NH3:%u", data->nh3_raw);
        snprintf(buf3, sizeof(buf3), "NO2:%u", data->no2_raw);
    } else {
        snprintf(buf1, sizeof(buf1), "GAS FAIL");
        snprintf(buf2, sizeof(buf2), "Waiting ...");
        snprintf(buf3, sizeof(buf3), "");
    }
    ssd1306_draw_string(disp, 2, 13, 2, buf1);
    ssd1306_draw_string(disp, 2, 30, 2, buf2);
    ssd1306_draw_string(disp, 2, 47, 2, buf3);

    ssd1306_show(disp);
}

void draw_screen_fourth(ssd1306_t *disp, const oled_flags_t *flags, const sensor_data_t *data) {
    char buf1[32];
    char buf2[32];

    ssd1306_clear(disp);
    draw_status_bar(disp, flags->wifi_ok, flags->gas_ok, flags->dht_ok);

    ntp_get_date_str(buf1, sizeof(buf1));
    ntp_get_time_str(buf2, sizeof(buf2));
    char* buf3 = getenv("TZ") ? tzname[0] : "UTC";

    ssd1306_draw_string(disp, 2, 13, 2, buf1);
    ssd1306_draw_string(disp, 2, 30, 2, buf2);
    ssd1306_draw_string(disp, 2, 47, 2, buf3);

    ssd1306_show(disp);
}

void draw_screen_fifth(ssd1306_t *disp, const oled_flags_t *flags, const sensor_data_t *data) {
    char buf1[32];
    char buf2[32];

    ssd1306_clear(disp);
    draw_status_bar(disp, flags->wifi_ok, flags->gas_ok, flags->dht_ok);

    draw_icon_1bit(disp, 2, 13, icon_temp_32, 32, 32);
    draw_icon_1bit(disp, 2, 46, icon_temp, 16, 16);
    draw_icon_1bit(disp, 35, 13, icon_hum, 16, 16);
    draw_icon_1bit(disp, 35, 46, icon_air, 16, 16);

    ssd1306_show(disp);
}

void draw_current_screen(ssd1306_t *disp,
                        oled_screen_t screen,
                        const oled_flags_t *flags,
                        const sensor_data_t *data) {
    switch (screen) {
        case SCREEN_MAIN:
            draw_screen_main(disp, flags, data);
            break;

        case SCREEN_SECOND:
            draw_screen_second(disp, flags, data);
            break;

        case SCREEN_THIRD:
            draw_screen_third(disp, flags, data);
            break;
        
        case SCREEN_FOURTH:
            draw_screen_fourth(disp, flags, data);
            break;

        case SCREEN_FIFTH:
            draw_screen_fifth(disp, flags, data);
            break;

        default:
            draw_screen_main(disp, flags, data);
            break;
    }
}

void oled_debug_mark(const char *msg) {
    char time_str[32];
    ntp_get_time_str(time_str, sizeof(time_str));
    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 0, 0, 1, "DBG:");
    ssd1306_draw_string(&disp, 0, 16, 1, msg);
    ssd1306_draw_string(&disp, 0, 32, 1, time_str);
    ssd1306_show(&disp);
}

void oled_debug_mark2(const char *msg1, const char *msg2) {
    char time_str[32];
    ntp_get_time_str(time_str, sizeof(time_str));
    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 0, 0, 1, msg1);
    ssd1306_draw_string(&disp, 0, 16, 1, msg2);
    ssd1306_draw_string(&disp, 0, 32, 1, time_str);
    ssd1306_show(&disp);
}