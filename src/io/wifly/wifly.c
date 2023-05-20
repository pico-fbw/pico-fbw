/**
 * Huge thank-you to Rasperry Pi (as a part of the Pico examples library) for providing much of the code used in Wi-Fly!
 * 
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "pcl/dhcp.h"
#include "pcl/dns.h"
#include "pcl/tcp.h"

#include "wifly.h"
#include "../../config.h"

dhcp_server_t dhcp_server;
dns_server_t dns_server;

void wifly_init() {
    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));
    if (!state) {
        ERROR_printf("failed to allocate state\n");
    }

    const char *ap_name = WIFLY_NETWORK_NAME;
    #ifdef WIFLY_NETWORK_USE_PASSWORD
        const char *password = WIFLY_NETWORK_PASSWORD;
    #else
        const char *password = NULL;
    #endif

    cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);

    ip4_addr_t mask;
    IP4_ADDR(ip_2_ip4(&state->gw), 192, 168, 4, 1);
    IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);

    // Start the dhcp server
    dhcp_server_init(&dhcp_server, &state->gw, &mask);
    // Start the dns server
    dns_server_init(&dns_server, &state->gw);

    if (!tcp_server_open(state)) {
        ERROR_printf("failed to open server\n");
    }
}

void wifly_deinit() {
    dns_server_deinit(&dns_server);
    dhcp_server_deinit(&dhcp_server);
    // cyw43_arch_deinit();
}
