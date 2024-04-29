/**
 * This file utilizes code under the MIT License. See "LICENSE" for details.
 */

/**
 * This file is part of the MicroPython project, http://micropython.org/
 * https://github.com/micropython/micropython/blob/master/shared/netutils/dhcpserver.h
 */

#pragma once

#include "platform/defs.h"

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include <stdbool.h>
#include "platform/types.h"

#include "lwip/ip_addr.h"
#include "lwip/udp.h"

#define DHCPS_BASE_IP (16)
#define DHCPS_MAX_IP (8)

// clang-format on

typedef struct DHCPLease {
    u8 mac[6];
    u16 expiry;
} DHCPLease;

typedef struct DHCPServer {
    ip_addr_t ip;
    ip_addr_t nm;
    DHCPLease lease[DHCPS_MAX_IP];
    struct udp_pcb *udp;
} DHCPServer;

/**
 * Initializes a DHCP server.
 * @param d the DHCP server
 * @param ip the IP address of the server
 * @param nm the netmask of the server
 * @return true if the server was initialized successfully
 */
bool dhcp_server_init(DHCPServer *d, ip_addr_t *ip, ip_addr_t *nm);

/**
 * Deinitializes a DHCP server.
 * @param d the DHCP server
 */
void dhcp_server_deinit(DHCPServer *d);

#endif // PLATFORM_SUPPORTS_WIFI
