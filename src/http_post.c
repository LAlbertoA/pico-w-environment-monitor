#include "http_post.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "debug_state.h"
#include "oled.h"

typedef struct {
    bool done;
    bool success;
    int err_code;
} http_result_t;

static err_t on_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    (void)arg;
    (void)tpcb;
    (void)len;
    return ERR_OK;
}

static void on_err(void *arg, err_t err) {
    http_result_t *res = (http_result_t *)arg;
    if (!res) return;

    res->done = true;
    res->success = false;
    res->err_code = err;

    g_dbg_phase = DBG_HTTP_FAIL;
    g_dbg_code = err;
}

static err_t on_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    http_result_t *res = (http_result_t *)arg;

    if (!p) {
        res->done = true;
        res->success = (err == ERR_OK);
        res->err_code = err;

        g_dbg_phase = res->success ? DBG_HTTP_SUCCESS : DBG_HTTP_FAIL;
        g_dbg_code = err;

        cyw43_arch_lwip_begin();
        tcp_arg(tpcb, NULL);
        tcp_sent(tpcb, NULL);
        tcp_recv(tpcb, NULL);
        tcp_err(tpcb, NULL);
        tcp_close(tpcb);
        cyw43_arch_lwip_end();

        return ERR_OK;
    }

    cyw43_arch_lwip_begin();
    tcp_recved(tpcb, p->tot_len);
    cyw43_arch_lwip_end();

    pbuf_free(p);
    return ERR_OK;
}

static err_t on_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    (void)tpcb;

    http_result_t *res = (http_result_t *)arg;
    if (err != ERR_OK) {
        res->done = true;
        res->success = false;
        res->err_code = err;

        g_dbg_phase = DBG_HTTP_FAIL;
        g_dbg_code = err;
        return err;
    }
    return ERR_OK;
}

bool http_post_text(const char *server_ip, int port,
                    const char *path, const char *body) {
    g_dbg_code = 0;
    g_dbg_phase = DBG_HTTP_TCP_NEW;
    oled_debug_mark("TCP_NEW");

    ip_addr_t addr;
    if (!ipaddr_aton(server_ip, &addr)) {
        g_dbg_phase = DBG_HTTP_FAIL;
        g_dbg_code = -1001;
        oled_debug_mark("BAD_IP");
        return false;
    }

    cyw43_arch_lwip_begin();
    struct tcp_pcb *pcb = tcp_new_ip_type(IP_GET_TYPE(&addr));
    cyw43_arch_lwip_end();

    if (!pcb) {
        g_dbg_phase = DBG_HTTP_FAIL;
        g_dbg_code = -1002;
        oled_debug_mark("NO_PCB");
        return false;
    }

    http_result_t result = { false, false, 0 };

    cyw43_arch_lwip_begin();
    tcp_arg(pcb, &result);
    tcp_recv(pcb, on_recv);
    tcp_sent(pcb, on_sent);
    tcp_err(pcb, on_err);
    cyw43_arch_lwip_end();

    g_dbg_phase = DBG_HTTP_TCP_CONNECT;
    oled_debug_mark("TCP_CONN");

    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(pcb, &addr, port, on_connected);
    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        g_dbg_phase = DBG_HTTP_FAIL;
        g_dbg_code = err;
        oled_debug_mark("CONN_FAIL");

        cyw43_arch_lwip_begin();
        tcp_abort(pcb);
        cyw43_arch_lwip_end();

        return false;
    }

    uint32_t start = to_ms_since_boot(get_absolute_time());
    bool request_sent = false;

    while (!result.done && (to_ms_since_boot(get_absolute_time()) - start < 5000)) {
        g_dbg_heartbeat++;

        if (!request_sent) {
            g_dbg_phase = DBG_HTTP_WAIT_EST;
            oled_debug_mark("WAIT_EST");
        }

        if (!request_sent && pcb->state == ESTABLISHED) {
            char req[512];
            int len = snprintf(req, sizeof(req),
                "POST %s HTTP/1.1\r\n"
                "Host: %s:%d\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %d\r\n"
                "Connection: close\r\n"
                "\r\n"
                "%s",
                path, server_ip, port, (int)strlen(body), body
            );

            g_dbg_phase = DBG_HTTP_TCP_WRITE;
            g_dbg_code = 0;
            oled_debug_mark("TCP_WRITE");

            cyw43_arch_lwip_begin();
            err_t werr = tcp_write(pcb, req, len, TCP_WRITE_FLAG_COPY);
            cyw43_arch_lwip_end();

            if (werr != ERR_OK) {
                g_dbg_phase = DBG_HTTP_FAIL;
                g_dbg_code = werr;
                oled_debug_mark("WR_FAIL");

                cyw43_arch_lwip_begin();
                tcp_abort(pcb);
                cyw43_arch_lwip_end();

                return false;
            }

            g_dbg_phase = DBG_HTTP_TCP_OUTPUT;
            oled_debug_mark("TCP_OUT");

            cyw43_arch_lwip_begin();
            err_t oerr = tcp_output(pcb);
            cyw43_arch_lwip_end();

            if (oerr != ERR_OK) {
                g_dbg_phase = DBG_HTTP_FAIL;
                g_dbg_code = oerr;
                oled_debug_mark("OUT_FAIL");

                cyw43_arch_lwip_begin();
                tcp_abort(pcb);
                cyw43_arch_lwip_end();

                return false;
            }

            request_sent = true;
            g_dbg_phase = DBG_HTTP_WAIT_DONE;
            oled_debug_mark("WAIT_DONE");
        }

        sleep_ms(10);
    }

    if (!result.done) {
        g_dbg_phase = DBG_HTTP_TIMEOUT;
        g_dbg_code = -1003;
        oled_debug_mark("TIMEOUT");

        cyw43_arch_lwip_begin();
        tcp_abort(pcb);
        cyw43_arch_lwip_end();

        return false;
    }

    if (result.success) {
        g_dbg_post_count++;
        g_dbg_phase = DBG_HTTP_SUCCESS;
        g_dbg_code = 0;
        oled_debug_mark("HTTP_OK");
    } else {
        g_dbg_phase = DBG_HTTP_FAIL;
        g_dbg_code = result.err_code;
        oled_debug_mark("HTTP_FAIL");
    }

    return result.success;
}