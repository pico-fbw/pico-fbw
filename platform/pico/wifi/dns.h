/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
 */

#pragma once

#include "platform/defs.h"

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include <stdbool.h>

#include "lwip/ip_addr.h"

// clang-format on

typedef struct dns_server_t_ {
    struct udp_pcb *udp;
    ip_addr_t ip;
} DNSServer;

/**
 * Initializes a DNS server on the given IP address.
 * @param d the DNS server
 * @param ip the IP address of the server
 * @return true if the server was initialized successfully
 */
bool dns_server_init(DNSServer *d, ip_addr_t *ip);
void dns_server_deinit(DNSServer *d);

#endif // PLATFORM_SUPPORTS_WIFI
