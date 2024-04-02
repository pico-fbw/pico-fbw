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

// clang-format on

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

/**
 * Sends the body of a response to a client.
 * @param pcb the lwIP protocol control block for the connection
 * @param state the connection state data
 * @return the error code of the operation
 */
static err_t tcp_send_body(struct tcp_pcb *pcb, TCPConnection *state) {
    // FIXME: chunked tranfer encoding, currently does not work. solution is to give lwip more memory, but this will not scale
    // it's also currently very unstable
    // FIXME: and captive portal on mobile is broken

    // u16 chunk_len = LWIP_MIN(state->result_len, TCP_MSS);
    u16 chunk_len = state->result_len;
    LWIP_DEBUGF(TCP_DEBUG, ("sending %d bytes\n", chunk_len));
    err_t wrerr = tcp_write(pcb, state->result, chunk_len, TCP_WRITE_FLAG_COPY);
    if (wrerr != ERR_OK)
        LWIP_DEBUGF(TCP_DEBUG, ("ERROR: failed to write body data %d\n", wrerr));
    return wrerr;
}

// lwIP callback. Will be called when TCP data has been successfully sent.
static err_t tcp_server_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
    TCPConnection *con_state = (TCPConnection *)arg;
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_server_sent %d\n", len));
    con_state->sent_len += len;

    if (con_state->sent_len < con_state->result_len) {
        // Still data left to send
        LWIP_DEBUGF(TCP_DEBUG, ("sending more data\n"));
        err_t wrerr = tcp_send_body(pcb, con_state);
        if (wrerr != ERR_OK) {
            LWIP_DEBUGF(TCP_DEBUG, ("ERROR: failed to write result data %d\n", wrerr));
            return wrerr;
        }
    } else {
        // All data sent, clean up and close the connection
        LWIP_DEBUGF(TCP_DEBUG, ("all data sent\n"));
        if (con_state->result) {
            free(con_state->result);
            con_state->result = NULL;
        }
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

    char *received = NULL; // Buffer to store the received data
    char *header = NULL;   // Buffer to store the header that will be sent to the client
    i32 header_len = 0;
    if (p->tot_len > 0) {
        LWIP_DEBUGF(TCP_DEBUG, ("tcp_server_recv %d err %d\n", p->tot_len, err));
        for (struct pbuf *q = p; q != NULL; q = q->next)
            LWIP_DEBUGF(TCP_INPUT_DEBUG, ("in: %.*s\n\n\n", q->len, (char *)q->payload));

        // Transfer the received data from the pbuf to our buffer
        received = malloc(p->tot_len + 1);
        if (!received) {
            LWIP_DEBUGF(TCP_DEBUG, ("ERROR: failed to allocate received buffer\n"));
            goto close;
        }
        pbuf_copy_partial(p, (void *)received, p->tot_len, 0);
        received[p->tot_len] = '\0';

        // Filter by request type
        if (strncmp(HTTP_GET, received, sizeof(HTTP_GET) - 1) == 0) { // -1 to ignore null terminator
            // GET request:
            // Extract the request (everything but "GET") and parameters
            char *request = received + sizeof(HTTP_GET); // no -1 because we want to skip the space

            // Call the on_get function provided by the http server to fetch the content
            if ((*con_state->on_get)(con_state, request, &header, &header_len, &con_state->result, &con_state->result_len) != 0)
                goto close;

        } else if (strncmp(HTTP_POST, received, sizeof(HTTP_POST) - 1) == 0) {
            // POST request:
            char *request = received + sizeof(HTTP_POST);
            if ((*con_state->on_post)(con_state, request, &header, &header_len, &con_state->result, &con_state->result_len) !=
                0)
                goto close;
        }
        // Request is now processed, time to send our response

        // Send the header to the client
        if (!header) {
            LWIP_DEBUGF(TCP_DEBUG, ("ERROR: no header data\n"));
            goto close;
        }
        if (header_len > TCP_SND_BUF) {
            LWIP_DEBUGF(TCP_DEBUG, ("ERROR: header too large %ld\n", header_len));
            goto close;
        }
        err_t wrerr = tcp_write(pcb, header, header_len, 0);
        if (wrerr != ERR_OK) {
            LWIP_DEBUGF(TCP_DEBUG, ("ERROR: failed to write header data (%d)\n", wrerr));
            goto close;
        }

        // Send the first chunk of body data, if any
        if (con_state->result) {
            con_state->sent_len = 0;
            if (con_state->result_len > 0) {
                if (tcp_send_body(pcb, con_state) != ERR_OK)
                    goto close;
            }
        }

        tcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);
    if (received)
        free(received);
    if (header)
        free(header);
    // Don't free con_state->result here, as it will be freed after the response is sent
    return ERR_OK;
close:
    if (received)
        free(received);
    if (header)
        free(header);
    return tcp_close_client_connection(con_state, pcb, err);
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
