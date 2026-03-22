#include "dht11.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

static uint g_dht_pin;

void dht11_init(uint pin) {
    g_dht_pin = pin;
}

static inline bool wait_for_level(uint pin, bool level, uint32_t timeout_us) {
    absolute_time_t t0 = get_absolute_time();
    while (gpio_get(pin) != (int)level) {
        if (absolute_time_diff_us(t0, get_absolute_time()) > timeout_us) return false;
    }
    return true;
}

bool dht11_read(int *temp_c, int *hum) {
    uint8_t data[5] = {0};

    // Start signal: pull low >= 18 ms
    gpio_init(g_dht_pin);
    gpio_set_dir(g_dht_pin, GPIO_OUT);
    gpio_put(g_dht_pin, 0);
    sleep_ms(20);

    // Pull high 20-40 us
    gpio_put(g_dht_pin, 1);
    sleep_us(30);

    // Switch to input
    gpio_set_dir(g_dht_pin, GPIO_IN);

    // Sensor response: low ~80us, then high ~80us
    if (!wait_for_level(g_dht_pin, 0, 200)) return false;
    if (!wait_for_level(g_dht_pin, 1, 200)) return false;
    if (!wait_for_level(g_dht_pin, 0, 200)) return false;

    for (int i = 0; i < 40; i++) {
        if (!wait_for_level(g_dht_pin, 1, 100)) return false;

        absolute_time_t t_high = get_absolute_time();
        if (!wait_for_level(g_dht_pin, 0, 120)) return false;

        int high_us = (int)absolute_time_diff_us(t_high, get_absolute_time());
        int bit = (high_us > 45) ? 1 : 0;

        data[i / 8] <<= 1;
        data[i / 8] |= (uint8_t)bit;
    }

    // checksum
    uint8_t sum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (sum != data[4]) return false;

    // DHT11: data[0]=RH int, data[2]=Temp int
    *hum = data[0];
    *temp_c = data[2];
    return true;
}