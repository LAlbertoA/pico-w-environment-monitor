#ifndef MICS6814_H
#define MICS6814_H

#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"

void mics6814_init(uint co_gpio, uint nh3_gpio, uint no2_gpio);
bool mics6814_read(uint16_t *co_raw, uint16_t *nh3_raw, uint16_t *no2_raw);
float mics6814_raw_to_volts(uint16_t raw);

#endif