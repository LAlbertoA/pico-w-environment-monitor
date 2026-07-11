#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <stdint.h>

typedef struct {
    char room_id[33];
    uint16_t device_id;
} device_config_t;

extern device_config_t g_device_config;

#endif // DEVICE_CONFIG_H