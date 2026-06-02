#include "sht40.h"

#include <stdint.h>
#include "pico/stdlib.h"

#define SHT40_CMD_MEASURE_HIGH_PRECISION 0xFD

static i2c_inst_t *g_sht40_i2c = NULL;
static uint8_t g_sht40_addr = SHT40_DEFAULT_ADDR;

static uint8_t sht40_crc8(const uint8_t *data, int len) {
    uint8_t crc = 0xFF;

    for (int i = 0; i < len; i++) {
        crc ^= data[i];

        for (int bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (uint8_t)((crc << 1) ^ 0x31);
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

void sht40_init(i2c_inst_t *i2c, uint8_t addr) {
    g_sht40_i2c = i2c;
    g_sht40_addr = addr;
    sleep_ms(2);  // power-up margin
}

bool sht40_read(float *temp_c, float *hum_rh) {
    if (!g_sht40_i2c || !temp_c || !hum_rh) {
        return false;
    }

    uint8_t cmd = SHT40_CMD_MEASURE_HIGH_PRECISION;

    int written = i2c_write_blocking(
        g_sht40_i2c,
        g_sht40_addr,
        &cmd,
        1,
        false
    );

    if (written != 1) {
        return false;
    }

    sleep_ms(10);

    uint8_t rx[6] = {0};

    int read = i2c_read_blocking(
        g_sht40_i2c,
        g_sht40_addr,
        rx,
        6,
        false
    );

    if (read != 6) {
        return false;
    }

    if (sht40_crc8(&rx[0], 2) != rx[2]) {
        return false;
    }

    if (sht40_crc8(&rx[3], 2) != rx[5]) {
        return false;
    }

    uint16_t raw_t  = ((uint16_t)rx[0] << 8) | rx[1];
    uint16_t raw_rh = ((uint16_t)rx[3] << 8) | rx[4];

    float t = -45.0f + 175.0f * ((float)raw_t / 65535.0f);
    float rh = -6.0f + 125.0f * ((float)raw_rh / 65535.0f);

    if (rh < 0.0f) rh = 0.0f;
    if (rh > 100.0f) rh = 100.0f;

    *temp_c = t;
    *hum_rh = rh;

    return true;
}