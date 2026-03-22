#include "wifi.h"
#include "pico/cyw43_arch.h"
#include <stdio.h>

#ifndef WIFI_SSID
#error WIFI_SSID not defined, set it in CMakeLists.txt
#endif

#ifndef WIFI_PASSWORD
#error WIFI_PASSWORD not defined, set it in CMakeLists.txt
#endif

bool wifi_init_and_connect(void) {
    if (cyw43_arch_init()) {
        printf("ERROR: cyw43_arch_init failed\n");
        return false;
    }

    cyw43_arch_enable_sta_mode();

    printf("Conectando a WiFi: %s\n", WIFI_SSID);

    int err = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID,
        WIFI_PASSWORD,
        CYW43_AUTH_WPA2_AES_PSK,
        30000
    );

    if (err) {
        printf("ERROR: fallo al conectar WiFi = %d\n", err);
        return false;
    }

    printf("WiFi conectado\n");
    return true;
}

bool wifi_reconnect(void) {
    if (wifi_is_connected()) return true;

    printf("WiFi desconectado, intentando reconectar...\n");
    int err =cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID,
        WIFI_PASSWORD,
        CYW43_AUTH_WPA2_AES_PSK,
        10000
    );

    if (err) {
        printf("ERROR: fallo al reconectar WiFi. err = %d\n", err);
        return false;
    }

    return wifi_is_connected();
}

bool wifi_is_connected(void) {
    int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    return status >= 0;
}