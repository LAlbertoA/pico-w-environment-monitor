#ifndef SHT40_H
#define SHT40_H

#include <stdbool.h>
#include "hardware/i2c.h"

#define SHT40_DEFAULT_ADDR 0x44

void sht40_init(i2c_inst_t *i2c, uint8_t addr);
bool sht40_read(float *temp_c, float *hum_rh);

#endif