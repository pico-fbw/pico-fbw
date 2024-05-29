#pragma once

#include "platform/defs.h"

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include <stdbool.h>
#include "lwip/ip_addr.h"

#include "platform/types.h"

// clang-format on

typedef struct TCPServer {
    struct tcp_pcb *server_pcb;
    ip_addr_t ip; // Gateway IP
} TCPServer;

typedef struct TCPConnection {
    struct tcp_pcb *pcb;
    ip_addr_t *ip;
    void *state;
} TCPConnection;

/**
 * Opens a lwIP TCP server on the given port.
 * @param state the TCP server state
 * @param ip the IP address to open the server on, or NULL for any
 * @param port the port to open the server on
 * @return true if the server was opened successfully
 */
bool tcp_server_open(TCPServer *state, ip_addr_t *ip, u16 port);

/**
 * Closes a lwIP TCP server.
 * @param state the TCP server state
 * @return true if the server was closed successfully
 */
bool tcp_server_close(TCPServer *state);

#endif // PLATFORM_SUPPORTS_WIFI
