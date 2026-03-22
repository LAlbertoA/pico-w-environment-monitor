#ifndef DHT11_H
#define DHT11_H

#include <stdbool.h>
#include "pico/stdlib.h"

void dht11_init(uint pin);
bool dht11_read(int *temp_c, int *hum);

#endif