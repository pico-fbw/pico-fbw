/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 * 
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
*/

/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <string.h>
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "dhcp.h"
#include "dns.h"

#include "../../../config.h"

#include "tcp.h"

#define TCP_PORT 80
#define POLL_TIME_S 5
#define HTTP_GET "GET"
#define HTTP_RESPONSE_HEADERS "HTTP/1.1 %d OK\nContent-Length: %d\nContent-Type: text/html; charset=utf-8\nConnection: close\n\n"
#define HTTP_RESPONSE_REDIRECT "HTTP/1.1 302 Redirect\nLocation: http://%s" REDIRECT "\n\n"
#define REDIRECT "/wifly"

// Keeps track of if we are using accumulated headers and if they are complete yet
static bool useAccHeaders = false;
static bool accHeadersFinal = false;

static err_t tcp_close_client_connection(TCP_CONNECT_STATE_T *con_state, struct tcp_pcb *client_pcb, err_t close_err) {
    if (client_pcb) {
        assert(con_state && con_state->pcb == client_pcb);
        tcp_arg(client_pcb, NULL);
        tcp_poll(client_pcb, NULL, 0);
        tcp_sent(client_pcb, NULL);
        tcp_recv(client_pcb, NULL);
        tcp_err(client_pcb, NULL);
        err_t err = tcp_close(client_pcb);
        if (err != ERR_OK) {
            FBW_DEBUG_printf("[tcp] ERROR: close failed %d, calling abort\n", err);
            tcp_abort(client_pcb);
            close_err = ERR_ABRT;
        }
        if (con_state) {
            free(con_state);
        }
    }
    return close_err;
}

void tcp_server_close(TCP_SERVER_T *state) {
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
}

static err_t tcp_server_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    TCP_DEBUG_printf("[tcp] tcp_server_sent %u\n", len);
    con_state->sent_len += len;
    if (con_state->sent_len >= con_state->header_len + con_state->result_len) {
        TCP_DEBUG_printf("[tcp] all done\n");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    return ERR_OK;
}

static int server_content(const char *request, const char *params, char *result, size_t max_result_len) {
    #if TCP_DUMP_DATA
        printf("[tcp] content params: %s\n", params);
    #endif
    int len = 0;
    // Check if we need to redirect instead of serving page content
    if (strncmp(request, REDIRECT, sizeof(REDIRECT) - 1) == 0) {
        // Check to see if we are expecting more packets, if so, hold generating any content until we recieve the full message
        if (useAccHeaders) {
            if (!accHeadersFinal) {
                return len;
            }
        }
        // If there are params, check to see if the flightplan data is there
        if (params) {
            if (strncmp(FPLAN_PARAM, params, sizeof(FPLAN_PARAM) - 1) == 0) {
                WIFLY_DEBUG_printf("[wifly] Flightplan submission detected, attempting to parse\n");
                wifly_parseFplan(params); // Status is now set internally inside of wifly.c not tcp
            }
        }
        // Generate page content based on the current status of the flightplan submission
        len = wifly_genPageContent(result, max_result_len);
    }
    return len;
}

static err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    if (!p) {
        TCP_DEBUG_printf("[tcp] connection closed\n");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    assert(con_state && con_state->pcb == pcb);
    if (p->tot_len > 0) {
        TCP_DEBUG_printf("[tcp] tcp_server_recv %d err %d\n", p->tot_len, err);
        #if TCP_DUMP_DATA
            for (struct pbuf *q = p; q != NULL; q = q->next) {
                printf("in: %.*s\n\n\n", q->len, q->payload);
            }
        #endif
        // Copy the request into the buffer
        pbuf_copy_partial(p, con_state->headers, p->tot_len > sizeof(con_state->headers) - 1 ? sizeof(con_state->headers) - 1 : p->tot_len, 0);

        // We check first to see if it is the final header in an accumulation
        if (useAccHeaders && (p->flags & PBUF_FLAG_PUSH)) {
            // Add the final header to the accumulated headers
            strcat(con_state->accHeaders, con_state->headers);
            // Null termination
            con_state->accHeaders[strlen(con_state->accHeaders)] = '\0';
            #if TCP_DUMP_DATA
                printf("[tcp] {acc} final header: %s\n", con_state->accHeaders);
            #endif
            // Set the final headers flag
            accHeadersFinal = true;
        }
        // Check if this is flagged as the last packet, if not, expect more packets; use accumulated headers
        if (!(p->flags & PBUF_FLAG_PUSH)) {
            useAccHeaders = true;
            // Dynamically allocate the header accumulator for headers
            // If we have not allocated any memory yet, do that, otherwise continue allocating more memory as we receive more packets
            if (con_state->accHeaders == NULL) {
                con_state->accHeaders = malloc((strlen(con_state->headers) + 1));
                if (con_state->accHeaders == NULL) {
                    // Memory allocation failure, close connection
                    return tcp_close_client_connection(con_state, pcb, ERR_MEM);
                }
                con_state->accHeaders[0] = '\0';
            } else {
                con_state->accHeaders = realloc(con_state->accHeaders, (strlen(con_state->headers) + strlen(con_state->accHeaders)));
                if (con_state->accHeaders == NULL) {
                    free(con_state->accHeaders);
                    useAccHeaders = false;
                    accHeadersFinal = false;
                    return tcp_close_client_connection(con_state, pcb, ERR_MEM);
                }
            }
            // Add the current headers to the accumulated headers
            strcat(con_state->accHeaders, con_state->headers);
            #if TCP_DUMP_DATA
                TCP_DEBUG_printf("[tcp] {acc} current header: %s\n", con_state->accHeaders);
            #endif
        }

        // Handle GET request
        if ((strncmp(HTTP_GET, con_state->headers, sizeof(HTTP_GET) - 1) == 0) || (useAccHeaders && (p->flags & PBUF_FLAG_PUSH))) {
            char *request = (accHeadersFinal ? con_state->accHeaders : con_state->headers) + sizeof(HTTP_GET); // + space
            char *params = strchr(request, '?');
            if (params) {
                if (*params) {
                    char *space = strchr(request, ' ');
                    *params++ = 0;
                    if (space) {
                        *space = 0;
                    }
                } else {
                    params = NULL;
                }
            }

            // Generate content
            con_state->result_len = server_content(request, params, con_state->result, sizeof(con_state->result));
            TCP_DEBUG_printf("[tcp] request: %s?%s\n", request, params);
            TCP_DEBUG_printf("[tcp] result_len: %d\n", con_state->result_len);

            // Check we had enough buffer space
            if (con_state->result_len > sizeof(con_state->result) - 1) {
                FBW_DEBUG_printf("[tcp] ERROR: too much result data %d\n", con_state->result_len);
                return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
            }

            // Generate web page
            if (con_state->result_len > 0) {
                con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_HEADERS,
                    200, con_state->result_len);
                if (con_state->header_len > sizeof(con_state->headers) - 1) {
                    FBW_DEBUG_printf("[tcp] ERROR: too much header data %d\n", con_state->header_len);
                    return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
                }
            } else {
                // Send redirect
                con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_REDIRECT,
                    ipaddr_ntoa(con_state->gw));
                TCP_DEBUG_printf("[tcp] sending redirect %s", con_state->headers);
            }

            // Send the headers to the client
            con_state->sent_len = 0;
            err_t err = tcp_write(pcb, con_state->headers, con_state->header_len, 0);
            if (err != ERR_OK) {
                FBW_DEBUG_printf("[tcp] ERROR: failed to write header data %d\n", err);
                return tcp_close_client_connection(con_state, pcb, err);
            }

            // Send the body to the client
            if (con_state->result_len) {
                err = tcp_write(pcb, con_state->result, con_state->result_len, 0);
                if (err != ERR_OK) {
                    FBW_DEBUG_printf("[tcp] ERROR: failed to write result data %d\n", err);
                    return tcp_close_client_connection(con_state, pcb, err);
                }
            }

            // Clean up accumulated headers if necessary
            if (accHeadersFinal) {
                // Free memory used by accumulated headers and reset accumulation flags
                free(con_state->accHeaders);
                con_state->accHeaders = NULL;
                useAccHeaders = false;
                accHeadersFinal = false;
            }
        }
        tcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);
    return ERR_OK;
}

static err_t tcp_server_poll(void *arg, struct tcp_pcb *pcb) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    TCP_DEBUG_printf("[tcp] tcp server polling\n");
    return tcp_close_client_connection(con_state, pcb, ERR_OK); // Just disconnect clent?
}

static void tcp_server_err(void *arg, err_t err) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    if (err != ERR_ABRT) {
        FBW_DEBUG_printf("[tcp] ERROR: %d\n", err);
        tcp_close_client_connection(con_state, con_state->pcb, err);
    }
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        FBW_DEBUG_printf("[tcp] failed to accept connection\n");
        return ERR_VAL;
    }
    TCP_DEBUG_printf("[tcp] client connected\n");

    // Create the state for the connection
    TCP_CONNECT_STATE_T *con_state = calloc(1, sizeof(TCP_CONNECT_STATE_T));
    if (!con_state) {
        FBW_DEBUG_printf("[tcp] ERROR: failed to allocate connect state\n");
        return ERR_MEM;
    }
    con_state->pcb = client_pcb; // for checking
    con_state->gw = &state->gw;

    // setup connection to client
    tcp_arg(client_pcb, con_state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, tcp_server_err);

    return ERR_OK;
}

bool tcp_server_open(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    TCP_DEBUG_printf("[tcp] starting server on port %u\n", TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        FBW_DEBUG_printf("[tcp] ERROR: failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, IP_ANY_TYPE, TCP_PORT);
    if (err) {
        FBW_DEBUG_printf("[tcp] ERROR: failed to bind to port %d\n");
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        FBW_DEBUG_printf("[tcp] ERROR: failed to listen\n");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);

    return true;
}
