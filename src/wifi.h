#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>

bool wifi_init_and_connect(void);
bool wifi_is_connected(void);
bool wifi_reconnect(void);

#endif