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
#include "debug.h"

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

    DBG_SET(DBG_HTTP_FAIL, err);
}

static err_t on_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    http_result_t *res = (http_result_t *)arg;

    if (!p) {
        res->done = true;
        res->success = (err == ERR_OK);
        res->err_code = err;

        DBG_SET(res->success ? DBG_HTTP_SUCCESS : DBG_HTTP_FAIL, err);

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

        DBG_SET(DBG_HTTP_FAIL, err);

        return err;
    }
    return ERR_OK;
}

bool http_post_text(const char *server_ip, int port,
                    const char *path, const char *body) {
    
    DBG_SET(DBG_HTTP_TCP_NEW, 0);
    DBG_OLED("TCP_NEW");

    ip_addr_t addr;
    if (!ipaddr_aton(server_ip, &addr)) {
        DBG_SET(DBG_HTTP_FAIL, -1001);
        DBG_OLED("BAD_IP");
        return false;
    }

    cyw43_arch_lwip_begin();
    struct tcp_pcb *pcb = tcp_new_ip_type(IP_GET_TYPE(&addr));
    cyw43_arch_lwip_end();

    if (!pcb) {
        DBG_SET(DBG_HTTP_FAIL, -1002);
        DBG_OLED("NO_PCB");
        return false;
    }

    http_result_t result = { false, false, 0 };

    cyw43_arch_lwip_begin();
    tcp_arg(pcb, &result);
    tcp_recv(pcb, on_recv);
    tcp_sent(pcb, on_sent);
    tcp_err(pcb, on_err);
    cyw43_arch_lwip_end();

    DBG_SET(DBG_HTTP_TCP_CONNECT, 0);
    DBG_OLED("TCP_CONN");

    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(pcb, &addr, port, on_connected);
    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        DBG_SET(DBG_HTTP_FAIL, err);
        DBG_OLED("CONN_FAIL");

        cyw43_arch_lwip_begin();
        tcp_abort(pcb);
        cyw43_arch_lwip_end();

        return false;
    }

    uint32_t start = to_ms_since_boot(get_absolute_time());
    bool request_sent = false;

    while (!result.done && (to_ms_since_boot(get_absolute_time()) - start < 5000)) {
        DBG_HEARTBEAT_INC();

        if (!request_sent) {
            DBG_SET(DBG_HTTP_WAIT_EST, 0);
            DBG_OLED("WAIT_EST");
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

            DBG_SET(DBG_HTTP_TCP_WRITE, 0);
            DBG_OLED("TCP_WRITE");

            cyw43_arch_lwip_begin();
            err_t werr = tcp_write(pcb, req, len, TCP_WRITE_FLAG_COPY);
            cyw43_arch_lwip_end();

            if (werr != ERR_OK) {

                DBG_SET(DBG_HTTP_FAIL, werr);
                DBG_OLED("WR_FAIL");

                cyw43_arch_lwip_begin();
                tcp_abort(pcb);
                cyw43_arch_lwip_end();

                return false;
            }

            DBG_SET(DBG_HTTP_TCP_OUTPUT, 0);
            DBG_OLED("TCP_OUT");

            cyw43_arch_lwip_begin();
            err_t oerr = tcp_output(pcb);
            cyw43_arch_lwip_end();

            if (oerr != ERR_OK) {

                DBG_SET(DBG_HTTP_FAIL, oerr);
                DBG_OLED("OUT_FAIL");

                cyw43_arch_lwip_begin();
                tcp_abort(pcb);
                cyw43_arch_lwip_end();

                return false;
            }

            request_sent = true;
            DBG_SET(DBG_HTTP_WAIT_DONE, 0);
            DBG_OLED("WAIT_DONE");
        }

        sleep_ms(10);
    }

    if (!result.done) {
        DBG_SET(DBG_HTTP_TIMEOUT, -1003);
        DBG_OLED("TIMEOUT");

        cyw43_arch_lwip_begin();
        tcp_abort(pcb);
        cyw43_arch_lwip_end();

        return false;
    }

    if (result.success) {
        g_dbg_post_count++;
        DBG_SET(DBG_HTTP_SUCCESS, 0);
        DBG_OLED("HTTP_OK");
    } else {
        DBG_SET(DBG_HTTP_FAIL, result.err_code);
        DBG_OLED("HTTP_FAIL");
    }

    return result.success;
}