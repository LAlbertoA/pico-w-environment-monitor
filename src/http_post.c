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
} http_result_t;

static err_t on_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    (void)arg;
    (void)tpcb;
    (void)len;
    return ERR_OK;
}

static err_t on_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    http_result_t *res = (http_result_t *)arg;

    if (!p) {
        res->done = true;
        res->success = (err == ERR_OK);
        g_dbg_phase = res->success ? DBG_HTTP_SUCCESS : DBG_HTTP_FAIL;
        g_dbg_code = err;
        tcp_close(tpcb);
        return ERR_OK;
    }

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

static err_t on_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    (void)tpcb;

    http_result_t *res = (http_result_t *)arg;
    if (err != ERR_OK) {
        res->done = true;
        res->success = false;
        g_dbg_phase = DBG_HTTP_FAIL;
        oled_debug_mark("HTTP_FAIL");
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
        oled_debug_mark("HTTP_FAIL");
        g_dbg_code = -1001;
        printf("IP invalida: %s\n", server_ip);
        return false;
    }

    struct tcp_pcb *pcb = tcp_new_ip_type(IP_GET_TYPE(&addr));
    if (!pcb) {
        g_dbg_phase = DBG_HTTP_FAIL;
        oled_debug_mark("HTTP_FAIL");
        g_dbg_code = -1002;
        printf("tcp_new fallo\n");
        return false;
    }

    http_result_t result = { false, false };

    tcp_arg(pcb, &result);
    tcp_recv(pcb, on_recv);
    tcp_sent(pcb, on_sent);


    g_dbg_phase = DBG_HTTP_TCP_CONNECT;
    oled_debug_mark("TCP_CONN");

    err_t err = tcp_connect(pcb, &addr, port, on_connected);
    if (err != ERR_OK) {
        g_dbg_phase = DBG_HTTP_FAIL;
        oled_debug_mark("HTTP_FAIL");
        g_dbg_code = err;
        printf("tcp_connect fallo: %d\n", err);
        tcp_abort(pcb);
        return false;
    }

    uint32_t start = to_ms_since_boot(get_absolute_time());
    bool request_sent = false;

    while (!result.done && to_ms_since_boot(get_absolute_time()) - start < 5000) {
        g_dbg_heartbeat++;
        cyw43_arch_poll();
        sleep_ms(10);

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
            oled_debug_mark("TCP_WRITE");
            err_t werr = tcp_write(pcb, req, len, TCP_WRITE_FLAG_COPY);
            if (werr != ERR_OK) {
                g_dbg_phase = DBG_HTTP_FAIL;
                oled_debug_mark("HTTP_FAIL");
                g_dbg_code = werr;
                printf("tcp_write fallo: %d\n", werr);
                tcp_abort(pcb);
                return false;
            }

            g_dbg_phase = DBG_HTTP_TCP_OUTPUT;
            oled_debug_mark("TCP_OUT");
            err_t oerr = tcp_output(pcb);
            if (oerr != ERR_OK) {
                g_dbg_phase = DBG_HTTP_FAIL;
                oled_debug_mark("HTTP_FAIL");
                g_dbg_code = oerr;
                printf("tcp_output fallo: %d\n", oerr);
                tcp_abort(pcb);
                return false;
            }

            request_sent = true;
            g_dbg_phase = DBG_HTTP_WAIT_DONE;
            oled_debug_mark("WAIT_DONE");
        }
    }

    if (result.success) {
        g_dbg_post_count++;
        g_dbg_phase = DBG_HTTP_SUCCESS;
        oled_debug_mark("HTTP_OK");
        g_dbg_code = 0;
    } else {
        g_dbg_phase = DBG_HTTP_FAIL;
        oled_debug_mark("HTTP_FAIL");
    }

    return result.success;
}