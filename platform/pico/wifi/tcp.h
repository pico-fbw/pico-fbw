#pragma once

#include "platform/defs.h"

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include <stdbool.h>
#include "platform/types.h"

#include "lwip/ip_addr.h"

// clang-format on

// Forward declaration of TCPConnection
typedef struct TCPConnection TCPConnection;

/**
 * The format of a request handler function. Will be called by the TCP server on certain requests (GET, POST, etc.).
 * @param[in] request the request string (with the initial request type removed [GET, POST, etc.])
 * @param[out] header a pointer to a buffer to store the response header in, or NULL if an error occurred
 * @param[out] header_len the length of the response header, or 0 if an error occurred
 * @param[out] body a pointer to a buffer to store the response body in, or NULL if an error occurred
 * @param[out] body_len the length of the response body, or 0 if an error occurred
 * @return 0 if the request was handled successfully, or an error code if an error occurred
 * @note The buffers will be NULL when the function is called, and must be heap allocated by the function.
 * It will be freed by the TCP server after the response is sent.
 */
typedef i32 (*request_handler)(const TCPConnection *state, const char *request, char **header, i32 *header_len, char **body,
                               i32 *body_len);

typedef struct TCPServer {
    struct tcp_pcb *server_pcb;
    ip_addr_t ip; // Gateway IP
    request_handler on_get, on_post;
} TCPServer;

typedef struct TCPConnection {
    struct tcp_pcb *pcb;
    i32 result_len, sent_len; // Signed, as these can also hold error codes
    char *result;
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
