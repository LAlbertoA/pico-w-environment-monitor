#ifndef DEBUG_STATE_H
#define DEBUG_STATE_H

#include <stdint.h>

#ifndef OLED_DEBUG
#define OLED_DEBUG 0
#endif

#if OLED_DEBUG
typedef enum {
    DBG_IDLE = 0,
    DBG_WIFI_CHECK,
    DBG_WIFI_RECONNECT,
    DBG_DHT_READ,
    DBG_DHT_POST,
    DBG_GAS_READ,
    DBG_GAS_POST,
    DBG_OLED_DRAW,
    DBG_HTTP_TCP_NEW,
    DBG_HTTP_TCP_CONNECT,
    DBG_HTTP_WAIT_EST,
    DBG_HTTP_TCP_WRITE,
    DBG_HTTP_TCP_OUTPUT,
    DBG_HTTP_WAIT_DONE,
    DBG_HTTP_TIMEOUT,
    DBG_HTTP_SUCCESS,
    DBG_HTTP_FAIL
} debug_phase_t;

extern volatile debug_phase_t g_dbg_phase;
extern volatile int g_dbg_code;
extern volatile uint32_t g_dbg_heartbeat;

const char *debug_phase_str(debug_phase_t p);
#endif

extern volatile uint32_t g_dbg_post_count;

#endif