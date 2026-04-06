#include <stdbool.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "ntp.h"
#include "oled.h"

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