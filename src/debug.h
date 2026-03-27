#ifndef DEBUG_H
#define DEBUG_H

#ifndef OLED_DEBUG
#define OLED_DEBUG 0
#endif

#include "debug_state.h"

#if OLED_DEBUG
#include "oled.h"

#define DBG_OLED(msg)      do { oled_debug_mark(msg); } while (0)
#define DBG_PHASE(x)       do { g_dbg_phase = (x); } while (0)
#define DBG_CODE(x)        do { g_dbg_code = (x); } while (0)
#define DBG_HEARTBEAT_INC() do { g_dbg_heartbeat++; } while (0)
#define DBG_SET(p, c)      do { g_dbg_phase = (p); g_dbg_code = (c); } while (0)

#else

#define DBG_OLED(msg)       do { } while (0)
#define DBG_PHASE(x)        do { } while (0)
#define DBG_CODE(x)         do { } while (0)
#define DBG_HEARTBEAT_INC() do { } while (0)
#define DBG_SET(p, c)       do { } while (0)

#endif

#endif