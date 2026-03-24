#include "ntp.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"

#define NTP_SERVER "pool.ntp.org"
#define NTP_PORT   123
#define NTP_MSG_LEN 48
#define NTP_DELTA 2208988800UL

// NTP refresh interval in miliseconds
#define NTP_REFRESH_MS (6UL * 60UL * 60UL * 1000UL)
// Petition timeout in miliseconds
#define NTP_REQUEST_TIMEOUT_MS 10000UL

typedef struct {
    ip_addr_t server_addr;
    struct udp_pcb *pcb;
    bool dns_pending;
    bool request_in_flight;
    bool synced;
    bool server_addr_valid;

    absolute_time_t request_deadline;
    absolute_time_t next_refresh;

    time_t epoch_at_sync;
    uint32_t ms_at_sync;
} ntp_state_t;

static ntp_state_t g_ntp = {0};

static void ntp_start_request(void);

static void ntp_set_epoch(time_t epoch) {
    g_ntp.epoch_at_sync = epoch;
    g_ntp.ms_at_sync = to_ms_since_boot(get_absolute_time());
    g_ntp.synced = true;
    g_ntp.request_in_flight = false;
    g_ntp.next_refresh = make_timeout_time_ms(NTP_REFRESH_MS);
}

static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                     const ip_addr_t *addr, u16_t port) {
    (void)arg;
    (void)pcb;

    if (!p) return;

    if (port == NTP_PORT &&
        g_ntp.server_addr_valid &&
        ip_addr_cmp(addr, &g_ntp.server_addr) &&
        p->tot_len == NTP_MSG_LEN) {

        uint8_t mode = pbuf_get_at(p, 0) & 0x7;
        uint8_t stratum = pbuf_get_at(p, 1);

        if (mode == 0x4 && stratum != 0) {
            uint8_t seconds_buf[4] = {0};
            pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);

            uint32_t seconds_since_1900 =
                ((uint32_t)seconds_buf[0] << 24) |
                ((uint32_t)seconds_buf[1] << 16) |
                ((uint32_t)seconds_buf[2] << 8)  |
                ((uint32_t)seconds_buf[3]);

            uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
            ntp_set_epoch((time_t)seconds_since_1970);
        }
    }

    pbuf_free(p);
}

static void ntp_send_packet(void) {
    cyw43_arch_lwip_begin();

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    if (p) {
        uint8_t *req = (uint8_t *)p->payload;
        memset(req, 0, NTP_MSG_LEN);
        req[0] = 0x1b; // client request

        udp_sendto(g_ntp.pcb, p, &g_ntp.server_addr, NTP_PORT);
        pbuf_free(p);
    }

    cyw43_arch_lwip_end();

    g_ntp.request_in_flight = true;
    g_ntp.request_deadline = make_timeout_time_ms(NTP_REQUEST_TIMEOUT_MS);
}

static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    (void)hostname;
    (void)arg;

    g_ntp.dns_pending = false;

    if (!ipaddr) {
        g_ntp.request_in_flight = false;
        g_ntp.next_refresh = make_timeout_time_ms(30000);
        return;
    }

    g_ntp.server_addr = *ipaddr;
    g_ntp.server_addr_valid = true;
    ntp_send_packet();
}

static void ntp_start_request(void) {
    if (!cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA)) {
        g_ntp.next_refresh = make_timeout_time_ms(10000);
        return;
    }

    if (!g_ntp.pcb) {
        cyw43_arch_lwip_begin();
        g_ntp.pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
        if (g_ntp.pcb) {
            udp_recv(g_ntp.pcb, ntp_recv, NULL);
        }
        cyw43_arch_lwip_end();

        if (!g_ntp.pcb) {
            g_ntp.next_refresh = make_timeout_time_ms(10000);
            return;
        }
    }

    if (g_ntp.server_addr_valid) {
        ntp_send_packet();
        return;
    }

    g_ntp.dns_pending = true;
    g_ntp.request_in_flight = true;

    int err = dns_gethostbyname(NTP_SERVER, &g_ntp.server_addr, ntp_dns_found, NULL);
    if (err == ERR_OK) {
        g_ntp.server_addr_valid = true;
        g_ntp.dns_pending = false;
        ntp_send_packet();
    } else if (err != ERR_INPROGRESS) {
        g_ntp.dns_pending = false;
        g_ntp.request_in_flight = false;
        g_ntp.next_refresh = make_timeout_time_ms(30000);
    }
}

void ntp_init(void) {
    memset(&g_ntp, 0, sizeof(g_ntp));
    g_ntp.next_refresh = make_timeout_time_ms(2000);
}

void ntp_poll(void) {
    if (g_ntp.request_in_flight && time_reached(g_ntp.request_deadline)) {
        g_ntp.request_in_flight = false;
        g_ntp.dns_pending = false;
        g_ntp.next_refresh = make_timeout_time_ms(30000);
    }

    if (!g_ntp.request_in_flight && time_reached(g_ntp.next_refresh)) {
        ntp_start_request();
    }
}

bool ntp_has_time(void) {
    return g_ntp.synced;
}

time_t ntp_get_epoch(void) {
    if (!g_ntp.synced) return 0;

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    return g_ntp.epoch_at_sync + (time_t)((now_ms - g_ntp.ms_at_sync) / 1000);
}

void ntp_get_hms(char *buf, size_t n) {
    if (!g_ntp.synced) {
        snprintf(buf, n, "--:--:--");
        return;
    }

    time_t now = ntp_get_epoch();
    struct tm *local = localtime(&now);

    if (!local) {
        snprintf(buf, n, "--:--:--");
        return;
    }

    snprintf(buf, n, "%02d/%02d/%04d %02d:%02d:%02d",
             local->tm_mday, local->tm_mon + 1, local->tm_year + 1900,
             local->tm_hour, local->tm_min, local->tm_sec);
}