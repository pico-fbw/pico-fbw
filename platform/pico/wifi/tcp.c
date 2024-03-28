/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
 */

// Big thanks to Raspberry Pi (as part of the pico-examples) for the baseline TCP server implementation on the Pico W!

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "tcp.h"

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lwip/debug.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#define POLL_TIME_S 5 // Interval to poll a TCP connection for activity

#define HTTP_GET "GET"
#define HTTP_POST "POST"

#define HTTP_RESPONSE_HEADER "HTTP/1.1 %d OK\nContent-Length: %ld\nContent-Type: %s\nContent-Encoding: gzip\nConnection: close\n\n"
#define HTTP_RESPONSE_HEADER_NOGZIP "HTTP/1.1 %d OK\nContent-Length: %ld\nContent-Type: %s\nConnection: close\n\n"

// If a content fetch within a GET request fails, the client will be redirected to this location
#define REDIRECT_LOCATION "/"
#define HTTP_RESPONSE_REDIRECT "HTTP/1.1 302 Redirect\nLocation: http://%s" REDIRECT_LOCATION "\n\n"

// clang-format on

/**
 * Gets the MIME type of a file based on its extension.
 * @param file the file name
 * @return the MIME type of the file
 */
const char *get_mime_type(const char *file) {
    const char *ext = strrchr(file, '.');
    if (ext == NULL) {
        // No "." in file name
        // Probably a directory, index.html will be returned
        return "text/html";
        // FIXME: better way of doing this? coordinate with the on_get function?
    }
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) {
        return "text/html";
    }
    if (strcmp(ext, ".js") == 0) {
        return "application/javascript";
    }
    if (strcmp(ext, ".css") == 0) {
        return "text/css";
    }
    if (strcmp(ext, ".svg") == 0) {
        return "image/svg+xml";
    }
    // Add more types as needed
    return "application/octet-stream";
}

/**
 * Closes a TCP connection with a client and cleans up any related data.
 * @param con_state the connection state data
 * @param client_pcb the lwIP protocol control block for the connection
 * @param close_err the error to close the connection with
 * @return the error that the connection was closed with (can be different from close_err)
 */
static err_t tcp_close_client_connection(TCPConnection *con_state, struct tcp_pcb *client_pcb, err_t close_err) {
    if (client_pcb) {
        assert(con_state && con_state->pcb == client_pcb);
        LWIP_DEBUGF(TCP_DEBUG, ("client disconnected, reason: %d\n", close_err));
        // Unregister callbacks and close the connection
        tcp_arg(client_pcb, NULL);
        tcp_poll(client_pcb, NULL, 0);
        tcp_sent(client_pcb, NULL);
        tcp_recv(client_pcb, NULL);
        tcp_err(client_pcb, NULL);
        err_t err = tcp_close(client_pcb);
        if (err != ERR_OK) {
            // Failed to gracefully close the connection, we must abort
            LWIP_DEBUGF(TCP_DEBUG, ("close failed (%d), aborting!\n", err));
            tcp_abort(client_pcb);
            close_err = ERR_ABRT;
        }
        if (con_state)
            free(con_state);
    }
    return close_err;
}

// lwIP callback. Will be called when TCP data has been successfully sent.
static err_t tcp_server_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
    TCPConnection *con_state = (TCPConnection *)arg;
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_server_sent %d\n", len));
    con_state->sent_len += len;
    if (con_state->sent_len >= con_state->header_len + con_state->result_len) {
        // All data has been sent, close the connection
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    return ERR_OK;
}

// lwIP callback. Will be called when TCP data is received on the connection.
// This is also where we send data back to the client.
static err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    TCPConnection *con_state = (TCPConnection *)arg;
    if (!p)
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    assert(con_state && con_state->pcb == pcb);
    if (p->tot_len > 0) {
        LWIP_DEBUGF(TCP_DEBUG, ("tcp_server_recv %d err %d\n", p->tot_len, err));
        for (struct pbuf *q = p; q != NULL; q = q->next)
            LWIP_DEBUGF(TCP_INPUT_DEBUG, ("in: %.*s\n\n\n", q->len, (char *)q->payload));

        // Transfer the received data from the pbuf to our buffer
        char *header = NULL;
        header = malloc(p->tot_len + 1);
        if (!header) {
            LWIP_DEBUGF(TCP_DEBUG, ("ERROR: failed to allocate header buffer\n"));
            return tcp_close_client_connection(con_state, pcb, ERR_MEM);
        }
        pbuf_copy_partial(p, (void *)header, p->tot_len, 0);
        header[p->tot_len] = '\0';

        char *result = NULL; // Will eventually hold the content to send back to the client

        // Filter by request type
        if (strncmp(HTTP_GET, header, sizeof(HTTP_GET) - 1) == 0) { // -1 to ignore null terminator
            // GET request:
            // Extract the request (everything but "GET") and parameters
            char *request = header + sizeof(HTTP_GET); // no -1 because we want to skip the space

            // Call the on_get function provided by the http server to fetch the content
            con_state->result_len = (*con_state->on_get)(request, &result);

            // Prepare the response header
            if (con_state->result_len > 0) {
                // Content was fetched successfully
                const char *mime_type = get_mime_type(request);
                con_state->header_len =
                    snprintf(header, p->tot_len, HTTP_RESPONSE_HEADER, 200, con_state->result_len, mime_type);
            } else if (con_state->result_len == 0) {
                // Content couldn't be fetched
                // Try redirecting the client back to the root?
                con_state->header_len = snprintf(header, p->tot_len, HTTP_RESPONSE_REDIRECT, ipaddr_ntoa(con_state->ip));
            } else {
                // Error while fetching content, close the connection
                LWIP_DEBUGF(TCP_DEBUG, ("ERROR: failed to fetch content %ld\n", con_state->result_len));
                return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
            }
        } else if (strncmp(HTTP_POST, header, sizeof(HTTP_POST) - 1) == 0) {
            // POST request:
            char *request = header + sizeof(HTTP_POST);

            con_state->result_len = (*con_state->on_post)(request, &result);

            if (con_state->result_len < 0) {
                LWIP_DEBUGF(TCP_DEBUG, ("ERROR: failed to fetch content %ld\n", con_state->result_len));
                return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
            }
            // All API responses are raw JSON (so no gzip)
            const char *mime_type = "application/json";
            con_state->header_len =
                snprintf(header, p->tot_len, HTTP_RESPONSE_HEADER_NOGZIP, 200, con_state->result_len, mime_type);
        }

        // Send the header and body to the client
        con_state->sent_len = 0;
        err_t wrerr = tcp_write(pcb, header, con_state->header_len, 0);
        if (wrerr != ERR_OK) {
            LWIP_DEBUGF(TCP_DEBUG, ("ERROR: failed to write header data %d\n", wrerr));
            return tcp_close_client_connection(con_state, pcb, wrerr);
        }
        if (con_state->result_len > 0) {
            LWIP_DEBUGF(TCP_DEBUG, ("sending %ld bytes\n", con_state->result_len));
            wrerr = tcp_write(pcb, result, con_state->result_len, TCP_WRITE_FLAG_COPY);
            if (wrerr != ERR_OK) {
                LWIP_DEBUGF(TCP_DEBUG, ("ERROR: failed to write result data %d\n", wrerr));
                return tcp_close_client_connection(con_state, pcb, wrerr);
            }
        }

        // Clean up
        free(header);
        free(result);
        tcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);
    return ERR_OK;
    (void)err;
}

// lwIP callback. Will be called when a connection is idle and needs to be polled.
static err_t tcp_server_poll(void *arg, struct tcp_pcb *pcb) {
    TCPConnection *con_state = (TCPConnection *)arg;
    LWIP_DEBUGF(TCP_DEBUG, ("tcp server polling\n"));
    return tcp_close_client_connection(con_state, pcb, ERR_OK);
}

// lwIP callback. Will be called when an error occurs on the connection.
static void tcp_server_err(void *arg, err_t err) {
    TCPConnection *con_state = (TCPConnection *)arg;
    if (err != ERR_ABRT) {
        LWIP_DEBUGF(TCP_DEBUG, ("ERROR: %d\n", err));
        tcp_close_client_connection(con_state, con_state->pcb, err);
    }
}

// lwIP callback. Will be called when a new connection is to be accepted.
static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCPServer *state = (TCPServer *)arg;
    LWIP_DEBUGF(TCP_DEBUG, ("client connected\n"));
    if (err != ERR_OK || client_pcb == NULL) {
        LWIP_DEBUGF(TCP_DEBUG, ("ERROR: couldn't accept connection\n"));
        return ERR_VAL;
    }

    // Create some data to help keep track of the connection
    TCPConnection *con_state = calloc(1, sizeof(TCPConnection));
    if (!con_state) {
        LWIP_DEBUGF(TCP_DEBUG, ("ERROR: failed to allocate connection state\n"));
        return ERR_MEM;
    }
    con_state->pcb = client_pcb;
    con_state->ip = &state->ip;
    con_state->on_get = &state->on_get;
    con_state->on_post = &state->on_post;

    // Set up callbacks
    tcp_arg(client_pcb, con_state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2); // https://doc.ecoscentric.com/ref/lwip-api-raw-tcp-poll.html
    tcp_err(client_pcb, tcp_server_err);

    return ERR_OK;
}

bool tcp_server_open(TCPServer *state, u16 port) {
    LWIP_DEBUGF(TCP_DEBUG, ("starting server on port %d\n", port));
    // First, create a temporary lwIP protocol control block
    // (the real one is created a few lines down using tcp_listen_with_backlog)
    struct tcp_pcb *tempPcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!tempPcb) {
        LWIP_DEBUGF(TCP_DEBUG, ("ERROR: failed to create PCB\n"));
        return false;
    }
    // Bind it to the TCP port, and start listening
    err_t err = tcp_bind(tempPcb, IP_ANY_TYPE, port);
    if (err) {
        LWIP_DEBUGF(TCP_DEBUG, ("ERROR: failed to bind PCB\n"));
        return false;
    }
    state->server_pcb = tcp_listen_with_backlog(tempPcb, 1);
    if (!state->server_pcb) {
        LWIP_DEBUGF(TCP_DEBUG, ("ERROR: failed to listen\n"));
        if (tempPcb)
            tcp_close(tempPcb);
        return false;
    }
    // Set up callbacks
    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);
    return true;
}

void tcp_server_close(TCPServer *state) {
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
}

#endif // PLATFORM_SUPPORTS_WIFI
