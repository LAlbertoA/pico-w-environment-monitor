#include "debug_state.h"

volatile debug_phase_t g_dbg_phase = DBG_IDLE;
volatile int g_dbg_code = 0;
volatile uint32_t g_dbg_heartbeat = 0;
volatile uint32_t g_dbg_post_count = 0;

const char *debug_phase_str(debug_phase_t p) {
    switch (p) {
        case DBG_IDLE: return "IDLE";
        case DBG_WIFI_CHECK: return "WIFI_CHK";
        case DBG_WIFI_RECONNECT: return "WIFI_REC";
        case DBG_DHT_READ: return "DHT_READ";
        case DBG_DHT_POST: return "DHT_POST";
        case DBG_GAS_READ: return "GAS_READ";
        case DBG_GAS_POST: return "GAS_POST";
        case DBG_OLED_DRAW: return "OLED";
        case DBG_HTTP_TCP_NEW: return "TCP_NEW";
        case DBG_HTTP_TCP_CONNECT: return "TCP_CONN";
        case DBG_HTTP_WAIT_EST: return "WAIT_EST";
        case DBG_HTTP_TCP_WRITE: return "TCP_WRITE";
        case DBG_HTTP_TCP_OUTPUT: return "TCP_OUT";
        case DBG_HTTP_WAIT_DONE: return "WAIT_DONE";
        case DBG_HTTP_TIMEOUT: return "TIMEOUT";
        case DBG_HTTP_SUCCESS: return "HTTP_OK";
        case DBG_HTTP_FAIL: return "HTTP_FAIL";
        default: return "UNKNOWN";
    }
}