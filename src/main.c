// main.c - Main application file for Pico W with DHT11, gas sensors, 
// and OLED display. This code reads from a DHT11 sensor and MICS6814 gas
// sensor, displays results on an OLED, and sends data to a server via WiFi.
// Author: Luis Arcos
// Date: 2024-06
// Note: This code is for educational purposes and may require adjustments
// for specific hardware setups.

// Standard C headers.
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// Pico SDK headers.
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"

// Hardware-specific headers for DHT11, MICS6814, OLED, WiFi.
#include "dht11.h"
#include "mics6814.h"
#include "ssd1306.h"
#include "wifi.h"
#include "http_post.h"
#include "debug_state.h"
#include "oled.h"
#include "ntp.h"
#include "buttons.h"

// Server configuration (can be overridden at compile time).
#ifndef SERVER_IP
#error SERVER_IP not defined, set it in CMakeLists.txt
#endif

// Server port for HTTP POST (can be overridden at compile time).
#ifndef SERVER_PORT
#error SERVER_PORT not defined, set it in CMakeLists.txt
#endif

// =============== DHT11 ===============
#define DHT_PIN 15

// ===== GAS (CJMCU-6814) ADC pins =====
#define NO2_GPIO 26
#define NH3_GPIO 27
#define CO_GPIO  28

// ============ I2C / OLED =============
#define I2C_PORT i2c0
#define SDA_PIN 4
#define SCL_PIN 5

// ============= Button(s) =============
#define BTN_PIN 22

ssd1306_t disp;

oled_flags_t status_flags = {0};
sensor_data_t sensor_data = {0};

// Global for multiple screens
static oled_screen_t current_screen = SCREEN_MAIN;

void update_ui(ssd1306_t *disp, const oled_flags_t *flags, const sensor_data_t *data) {

    if (button_was_pressed_event(BTN_PIN)) {
        current_screen = (current_screen + 1) % SCREEN_COUNT;
        draw_current_screen(disp, current_screen, flags, data);
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000);

    // Initialize NTP for timekeeping before anything else.
    ntp_init();
    setenv("TZ", "CST6", 1);
    tzset();

    // Initialize WiFi, sensors, and OLED display.
    dht11_init(DHT_PIN);
    mics6814_init(CO_GPIO, NH3_GPIO, NO2_GPIO);

    i2c_init(I2C_PORT, 400000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    oled_init_display(&disp, I2C_PORT);

    // Initialize button input
    button_init(BTN_PIN);

    //bool wifi_ok = wifi_init_and_connect();
    bool screen_thingy = true;

    //int t = 0, h = 0;
    //bool dht_ok = false;
    //bool gas_ok = false;
    //uint16_t co_raw = 0, nh3_raw = 0, no2_raw = 0;

    status_flags.wifi_ok = wifi_init_and_connect();
    status_flags.dht_ok  = false;
    status_flags.gas_ok  = false;

    sensor_data.t = 0;
    sensor_data.h = 0;
    sensor_data.co_raw  = 0;
    sensor_data.nh3_raw = 0;
    sensor_data.no2_raw = 0;

    absolute_time_t next_dht      = make_timeout_time_ms(10000);
    absolute_time_t next_gas      = make_timeout_time_ms(4000);
    absolute_time_t next_wifi_chk = make_timeout_time_ms(5000);
    absolute_time_t next_oled     = make_timeout_time_ms(1000);

    while (true) {

        // Check WiFi connection every 5 seconds and attempt reconnection if needed.
        if (time_reached(next_wifi_chk)) {
            status_flags.wifi_ok = wifi_is_connected();
            if (!status_flags.wifi_ok) {
                printf("WiFi desconectado, intentando reconectar...\n");
                status_flags.wifi_ok = wifi_reconnect();
                printf("Reconexion %s\n", status_flags.wifi_ok ? "EXITOSA" : "FALLIDA");
            }
            next_wifi_chk = delayed_by_ms(next_wifi_chk, 5000);
        }

        // Read DHT11 every 10 seconds and POST data if WiFi is connected.
        if (time_reached(next_dht)) {
            status_flags.dht_ok = dht11_read(&sensor_data.t, &sensor_data.h);
            if (status_flags.wifi_ok && status_flags.dht_ok) {
                char body[64];
                snprintf(body, sizeof(body), "DHT,T=%d,H=%d\n", sensor_data.t, sensor_data.h);
                bool ok = http_post_text(SERVER_IP, SERVER_PORT, "/dht", body);
                if (!ok) {
                    status_flags.wifi_ok = wifi_is_connected(); // Check if failure was due to WiFi disconnect
                }
                printf("POST /dht => %s | %s\n", ok ? "OK" : "FAIL", body);
            }
            next_dht = delayed_by_ms(next_dht, 10000);
        }

        // Read MICS6814 gas sensor every 4 seconds and POST data if WiFi is connected.
        if (time_reached(next_gas)) {
            status_flags.gas_ok = mics6814_read(&sensor_data.co_raw, &sensor_data.nh3_raw, &sensor_data.no2_raw);

            if (status_flags.gas_ok) {
                float co_v  = mics6814_raw_to_volts(sensor_data.co_raw);
                float nh3_v = mics6814_raw_to_volts(sensor_data.nh3_raw);
                float no2_v = mics6814_raw_to_volts(sensor_data.no2_raw);

                if (status_flags.wifi_ok) {
                    char body[128];
                    snprintf(body, sizeof(body),
                            "GAS,CO=%u,NH3=%u,NO2=%u,COv=%.3f,NH3v=%.3f,NO2v=%.3f\n",
                            sensor_data.co_raw, sensor_data.nh3_raw, sensor_data.no2_raw, co_v, nh3_v, no2_v);
                    bool ok = http_post_text(SERVER_IP, SERVER_PORT, "/gas", body);
                    if (!ok) {
                        status_flags.wifi_ok = wifi_is_connected(); // Check if failure was due to WiFi disconnect
                    }
                    printf("POST /gas => %s | %s\n", ok ? "OK" : "FAIL", body);
                }
            }

            next_gas = delayed_by_ms(next_gas, 4000);
        }

        // Update OLED display every second with latest sensor readings and status.
        if (time_reached(next_oled)) {
            //char line1[32], line2[32], line3[32];
#if OLED_DEBUG
            char dbg1[32], dbg2[32];

            snprintf(dbg1, sizeof(dbg1), "%s", debug_phase_str(g_dbg_phase));
            snprintf(dbg2, sizeof(dbg2), "E:%d P:%lu", g_dbg_code, g_dbg_post_count);
#endif

            ntp_poll(); // Poll NTP to keep time updated for display

            //ssd1306_clear(&disp);
            //draw_status_bar(&disp, wifi_ok, gas_ok, dht_ok);
            //ssd1306_draw_string(&disp, 2, 12, 1, wifi_ok ? "WiFi OK" : "WiFi FAIL");
            draw_current_screen(&disp, current_screen, &status_flags, &sensor_data);

#if OLED_DEBUG
            ssd1306_draw_string(&disp, 2, 52, 1, dbg1);
            ssd1306_draw_string(&disp, 56, 52, 1, dbg2);
#endif
            if (screen_thingy) {
                ssd1306_draw_square(&disp, 100, 15, 24, 24);
                screen_thingy = false;
            } else {
                ssd1306_draw_empty_square(&disp, 100, 15, 23, 23);
                screen_thingy = true;
            }
            //if (dht_ok) {
            //    snprintf(line1, sizeof(line1), "T:%dC H:%d%%", t, h);
            //} else {
            //    snprintf(line1, sizeof(line1), "DHT FAIL");
            //}

            //snprintf(line2, sizeof(line2), "CO:%u NH3:%u", co_raw, nh3_raw);
            //snprintf(line3, sizeof(line3), "NO2:%u", no2_raw);

            //ssd1306_draw_string(&disp, 2, 22, 1, line1);
            //ssd1306_draw_string(&disp, 2, 32, 1, line2);
            //ssd1306_draw_string(&disp, 2, 42, 1, line3);
            ssd1306_show(&disp);

            next_oled = delayed_by_ms(next_oled, 1000);
        }
        update_ui(&disp, &status_flags, &sensor_data);
        tight_loop_contents();
    }
}