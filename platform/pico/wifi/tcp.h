#pragma once

#include "platform/defs.h"

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include <stdbool.h>
#include "platform/int.h"

#include "lwip/ip_addr.h"

// clang-format on

/**
 * The format of a request handler function. Will be called by the TCP server on certain requests (GET, POST, etc.).
 * @param[in] request the request string (with the initial request type removed [GET, POST, etc.])
 * @param[out] result a pointer to a buffer to store the result in, or NULL if an error occurred
 * @return the length of the result, or an error code
 * @note The buffer will be NULL when the function is called, and must be heap allocated by the function.
 * It will be freed by the TCP server after the response is sent.
 */
typedef int (*request_handler)(const char *request, char **result);

typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    ip_addr_t ip; // Gateway IP
    request_handler on_get, on_post;
} TCPServer;

typedef struct TCP_CONNECT_STATE_T_ {
    struct tcp_pcb *pcb;
    i32 sent_len; // Signed, as these can also hold error codes
    i32 header_len;
    i32 result_len;
    // Pointers to server state
    ip_addr_t *ip;
    request_handler *on_get, *on_post;
} TCPConnection;

/**
 * Opens a lwIP TCP server on the given port.
 * @param state the HTTP server state
 * @param port the port to open the server on
 * @return true if the server was opened successfully
 */
bool tcp_server_open(TCPServer *state, u16 port);

/**
 * Closes a lwIP TCP server.
 * @param state the HTTP server state
 */
void tcp_server_close(TCPServer *state);

#endif // PLATFORM_SUPPORTS_WIFI
