#pragma once

#include "platform/defs.h"

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include <stdbool.h>

#include "lwip/ip_addr.h"

#include "tcp.h"

// clang-format on

typedef TCPServer HTTPServer; // Shh they'll never know

/**
 * Initializes an HTTP server on the given port. It will then listen for incoming connections.
 * @param server the HTTP server
 * @param ip the IP address of the server (gateway)
 * @return true if the server was initialized successfully
 */
bool http_server_init(HTTPServer *server, ip_addr_t *ip);

/**
 * Closes an HTTP server.
 * @param server the HTTP server
 */
void http_server_close(HTTPServer *server);

#endif // PLATFORM_SUPPORTS_WIFI
