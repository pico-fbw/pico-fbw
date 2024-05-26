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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lwip/debug.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "lib/mimetype.h"

#include "platform/flash.h"

#include "sys/api/api.h"
#include "sys/api/cmds/GET/get_config.h"
#include "sys/api/cmds/GET/get_info.h"
#include "sys/api/cmds/SET/set_config.h"
#include "sys/api/cmds/SET/set_flightplan.h"

#define CHUNK_XFER_SIZE 1024 // Size of each chunk to send in a chunked transfer
#define POLL_TIME_S 5 // Interval to poll a TCP connection for activity

#define TYPE_JSON "application/json"

#define API_V1_PATH "/api/v1/"
#define HTTP_GET "GET"
#define HTTP_POST "POST"

#define HEADER_302 "HTTP/1.1 302 Redirect\r\nLocation: http://%s/\r\nContent-Length: 0\r\n\r\n"
#define HEADER_404 "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n"
#define HEADER_500 "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n"

// clang-format on

typedef struct FileState {
    char *path;
    i32 offset;
} FileState;

/* --- Miscellaneous helpers --- */

/**
 * Extracts the URI of an HTTP request.
 * @param req the HTTP request
 * @param method the HTTP method used in the request
 * @return the URI from the request
 * @note The URI will be allocated by this function and must be freed by the caller.
 */
static char *extract_uri(const char *req, char *method) {
    const char *request = req + strlen(method) + 1; // +1 to skip the space
    char *end = strchr(request, ' ');
    if (end == NULL)
        return NULL;
    size_t uri_length = end - request;
    char *uri = malloc(uri_length + 1);
    if (uri != NULL) {
        strncpy(uri, request, uri_length);
        uri[uri_length] = '\0';
    }
    return uri;
}

/**
 * Extracts the body of an HTTP request.
 * @param req the HTTP request
 * @return the body of the request, or NULL if there is no body
 */
static const char *get_request_body(const char *req) {
    const char *body = strstr(req, "\r\n\r\n");
    if (body && *(body + 4) != '\0')
        return body + 4;
    return NULL;
}

/**
 * Creates an HTTP response.
 * @param status the HTTP status code, as a string (e.g. "200 OK")
 * @param content_type the content type of the response
 * @param body the body of the response, or NULL if not needed
 * @return the response, or NULL on error
 * @note The response will be allocated by this function and must be freed by the caller.
 */
static char *create_response(const char *status, const char *content_type, const char *body) {
    char contentLength[32];
    if (body)
        snprintf(contentLength, sizeof(contentLength), "%u", strlen(body));
    else
        strcpy(contentLength, "0");

    char *response =
        malloc(strlen("HTTP/1.1 ") + strlen(status) + strlen("\r\nContent-Type: ") + strlen(content_type) +
               strlen("\r\nContent-Length: ") + strlen(contentLength) + strlen("\r\n\r\n") + (body ? strlen(body) : 0) + 1);
    if (!response)
        return NULL;
    strcpy(response, "HTTP/1.1 ");
    strcat(response, status);
    strcat(response, "\r\nContent-Type: ");
    strcat(response, content_type);
    strcat(response, "\r\nContent-Length: ");
    strcat(response, contentLength);
    strcat(response, "\r\n\r\n");
    if (body)
        strcat(response, body);
    return response;
}

/**
 * Sends a chunk of a file to a client.
 * @param con_state the connection state data
 * @param pcb the lwIP protocol control block for the connection
 * @param path the path to the file to send
 * @param offset the offset in the file to start sending from
 * @return positive (# of bytes sent), 0 if there are no more bytes to send, or negative on error
 */
static i32 send_file_chunk(TCPConnection *con_state, struct tcp_pcb *pcb, const char *path, i32 offset) {
    lfs_file_t file;
    i32 err = lfs_file_open(&wwwfs, &file, path, LFS_O_RDONLY);
    LWIP_DEBUGF(TCP_DEBUG, ("send_file_chunk(%p, %p, %s, %ld): lfs_open -> %ld\n", con_state, pcb, path, offset, err));
    if (err != LFS_ERR_OK) {
        LWIP_DEBUGF(TCP_DEBUG, ("send_file_chunk: failed to open file (%ld)\n", err));
        return err;
    }
    if (offset >= lfs_file_size(&wwwfs, &file)) {
        LWIP_DEBUGF(TCP_DEBUG, ("send_file_chunk: no more data\n"));
        lfs_file_close(&wwwfs, &file);
        return 0;
    }
    lfs_file_seek(&wwwfs, &file, offset, LFS_SEEK_SET);
    char chunk[CHUNK_XFER_SIZE];
    i32 bytesRead = lfs_file_read(&wwwfs, &file, chunk, sizeof(chunk));
    LWIP_DEBUGF(TCP_DEBUG, ("send_file_chunk: read %ld bytes\n", bytesRead));
    if (bytesRead < 0) {
        lfs_file_close(&wwwfs, &file);
        return -1;
    }
    char chunkBegin[16] = "";
    snprintf(chunkBegin, sizeof(chunkBegin), "%lx\r\n", bytesRead);
    // No need to use TCP_WRITE_FLAG_COPY, since the chunk data will not be modified until it is sent anyway
    tcp_write(pcb, chunkBegin, strlen(chunkBegin), 0);
    tcp_write(pcb, chunk, bytesRead, 0);
    tcp_write(pcb, "\r\n", strlen("\r\n"), 0);
    lfs_file_close(&wwwfs, &file);
    return bytesRead;
}

/* --- API GET handlers --- */

static bool handle_api_v1_get_config(TCPConnection *con_state, struct tcp_pcb *pcb, const char *req) {
    char *out = NULL;
    // Perform the relavent API call using the request body and obtain its output
    i32 res = api_handle_get_config(get_request_body(req), &out);
    if (!out) {
        tcp_write(pcb, HEADER_500, strlen(HEADER_500), 0);
        return false;
    }
    // Create and send the HTTP response
    char *resp = create_response(api_res_to_http_status(res), TYPE_JSON, out);
    if (!resp) {
        free(out);
        tcp_write(pcb, HEADER_500, strlen(HEADER_500), 0);
        return false;
    }
    // Ensure to copy the response data into lwIP's memory space, since we free it immediately,
    // but lwIP may not have sent it yet
    tcp_write(pcb, resp, strlen(resp), TCP_WRITE_FLAG_COPY);
    free(resp);
    free(out);
    return res < 500 ? true : false;
    (void)con_state;
}

static bool handle_api_v1_get_info(TCPConnection *con_state, struct tcp_pcb *pcb, const char *req) {
    char *out = NULL;
    i32 res = api_handle_get_info(&out);
    if (!out) {
        tcp_write(pcb, HEADER_500, strlen(HEADER_500), 0);
        return false;
    }
    char *resp = create_response(api_res_to_http_status(res), TYPE_JSON, out);
    if (!resp) {
        free(out);
        tcp_write(pcb, HEADER_500, strlen(HEADER_500), 0);
        return false;
    }
    tcp_write(pcb, resp, strlen(resp), TCP_WRITE_FLAG_COPY);
    free(resp);
    free(out);
    return res < 500 ? true : false;
    (void)con_state;
    (void)req;
}

/* --- API SET handlers --- */

static bool handle_api_v1_set_config(TCPConnection *con_state, struct tcp_pcb *pcb, const char *req) {
    i32 res = api_handle_set_config(get_request_body(req));
    char *resp = create_response(api_res_to_http_status(res), TYPE_JSON, "{}");
    if (!resp) {
        tcp_write(pcb, HEADER_500, strlen(HEADER_500), 0);
        return false;
    }
    tcp_write(pcb, resp, strlen(resp), TCP_WRITE_FLAG_COPY);
    free(resp);
    return res < 500 ? true : false;
    (void)con_state;
}

static bool handle_api_v1_set_flightplan(TCPConnection *con_state, struct tcp_pcb *pcb, const char *req) {
    char *out = NULL;
    i32 res = api_handle_set_flightplan(get_request_body(req), &out);
    if (!out) {
        tcp_write(pcb, HEADER_500, strlen(HEADER_500), 0);
        return false;
    }
    char *resp = create_response(api_res_to_http_status(res), TYPE_JSON, out);
    if (!resp) {
        free(out);
        tcp_write(pcb, HEADER_500, strlen(HEADER_500), 0);
        return false;
    }
    tcp_write(pcb, resp, strlen(resp), TCP_WRITE_FLAG_COPY);
    free(resp);
    free(out);
    return res < 500 ? true : false;
    (void)con_state;
}

/* --- API MISC handlers --- */

static bool handle_api_v1_ping(TCPConnection *con_state, struct tcp_pcb *pcb, const char *req) {
    char *resp = create_response("200 OK", TYPE_JSON, "{}");
    if (!resp) {
        tcp_write(pcb, HEADER_500, strlen(HEADER_500), 0);
        return false;
    }
    tcp_write(pcb, resp, strlen(resp), TCP_WRITE_FLAG_COPY);
    free(resp);
    return true;
    (void)con_state;
    (void)req;
}

// Generic GET handler.
// Fetches the content requested by a GET request from littlefs and responds with the content.
// Will be called by the TCP server when a GET request is received that doesn't match any of the API paths.
static bool handle_common_get(TCPConnection *con_state, struct tcp_pcb *pcb, const char *req) {
    // This function is very similar to the esp32's handle_common_get, so take a look at that for more details/documentation
    char *uri = extract_uri(req, HTTP_GET);
    size_t pathSize = strlen(uri) + sizeof("/www") + sizeof("index.html") + sizeof(".gz");
    char *path = malloc(pathSize);
    if (!path) {
        free(uri);
        tcp_write(pcb, HEADER_500, strlen(HEADER_500), 0);
        return false;
    }
    strcpy(path, "/www");
    if (uri[strlen(uri) - 1] == '/')
        strlcat(path, "/index.html", pathSize);
    else
        strlcat(path, uri, pathSize);
    free(uri);
    LWIP_DEBUGF(TCP_DEBUG, ("handle_common_get: GET path: %s\n", path));

    bool gzipped = false;
    lfs_file_t file;
    i32 err;
    for (u32 i = 0; i < 2; i++) {
        err = lfs_file_open(&wwwfs, &file, path, LFS_O_RDONLY);
        if (err == LFS_ERR_NOENT && !gzipped) {
            strcat(path, ".gz");
            gzipped = true;
            continue;
        }
        break;
    }
    if (err != LFS_ERR_OK) {
        LWIP_DEBUGF(TCP_DEBUG, ("handle_common_get: file %s not found\n", path));
        free(path);
        return false;
    } else
        lfs_file_close(&wwwfs, &file);

    // Create a state object to keep track of the file transfer
    // This is because the transfer happens in chunks, and later chunks are handled in the tcp_server_sent callback,
    // so it needs to know the file path and the current offset in the file to continue sending the file
    con_state->state = malloc(sizeof(FileState));
    if (!con_state->state) {
        lfs_file_close(&wwwfs, &file);
        free(path);
        tcp_write(pcb, HEADER_500, strlen(HEADER_500), 0);
        return false;
    }
    FileState *state = (FileState *)con_state->state;
    state->path = strdup(path);
    state->offset = 0;

    // Construct and send the HTTP header
    char header[512] = "HTTP/1.1 200 OK\r\n";
    if (gzipped) {
        path[strlen(path) - 3] = '\0';
        strcat(header, "Content-Encoding: gzip\r\n");
    }
    strcat(header, "Content-Type: ");
    strcat(header, get_content_type(path));
    strcat(header, "\r\n");
    strcat(header, "Transfer-Encoding: chunked\r\n");
    strcat(header, "\r\n");
    LWIP_DEBUGF(TCP_DEBUG, ("handle_common_get: sending header:\n%s\n", header));
    tcp_write(pcb, header, strlen(header), 0);

    // Send the first chunk of the file
    // As mentioned earlier, subsequent chunks will be sent in the tcp_server_sent callback until the entire file is sent
    i32 sent = send_file_chunk(con_state, pcb, state->path, state->offset);
    if (sent <= 0) {
        free(state->path);
        free(state);
        free(path);
        return false;
    }
    state->offset += sent;

    free(path);
    return true;
}

/* --- High-level request handling --- */

static bool handle_request(TCPConnection *con_state, struct tcp_pcb *pcb, const char *request) {
    bool res = false;
    char *uri = NULL;
    // Filter by request type
    if (strncmp(HTTP_GET, request, strlen(HTTP_GET)) == 0) {
        // Filter based on the URI
        uri = extract_uri(request, HTTP_GET);
        if (!uri) {
            tcp_write(pcb, HEADER_500, strlen(HEADER_500), 0);
            return false;
        }
        LWIP_DEBUGF(TCP_DEBUG, ("handle_request: GET URI: %s\n", uri));
        if (strncmp(uri, API_V1_PATH, strlen(API_V1_PATH)) == 0) {
            if (strcmp(uri + strlen(API_V1_PATH), "get/config") == 0)
                res = handle_api_v1_get_config(con_state, pcb, request);
            else if (strcmp(uri + strlen(API_V1_PATH), "get/info") == 0)
                res = handle_api_v1_get_info(con_state, pcb, uri);
            else if (strcmp(uri + strlen(API_V1_PATH), "ping") == 0)
                res = handle_api_v1_ping(con_state, pcb, request);
        } else {
            // No other requests mathed, so it's probably a request for a file
            res = handle_common_get(con_state, pcb, request);
        }
    } else if (strncmp(HTTP_POST, request, strlen(HTTP_POST)) == 0) {
        uri = extract_uri(request, HTTP_POST);
        if (!uri) {
            tcp_write(pcb, HEADER_500, strlen(HEADER_500), 0);
            return false;
        }
        LWIP_DEBUGF(TCP_DEBUG, ("handle_request: POST URI: %s\n", uri));
        if (strncmp(uri, API_V1_PATH, strlen(API_V1_PATH)) == 0) {
            if (strcmp(uri + strlen(API_V1_PATH), "get/config") == 0)
                // GET_CONFIG can also be called with a POST request (in addition to a GET request, handled above)
                res = handle_api_v1_get_config(con_state, pcb, request);
            else if (strcmp(uri + strlen(API_V1_PATH), "set/config") == 0)
                res = handle_api_v1_set_config(con_state, pcb, request);
            else if (strcmp(uri + strlen(API_V1_PATH), "set/flightplan") == 0)
                res = handle_api_v1_set_flightplan(con_state, pcb, request);
        }
    }
    free(uri);
    if (!res) {
        // Redirect the client to the index page; this provides the captive portal behavior
        char header[sizeof(HEADER_302) + 32] = "";
        snprintf(header, sizeof(header), HEADER_302, ipaddr_ntoa(con_state->ip));
        tcp_write(pcb, header, strlen(header), 0);
    }
    return res;
}

/* --- Networking (lwIP) functions --- */

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

    // There is an ongoing send operation
    if (con_state->state) {
        FileState *state = (FileState *)con_state->state;
        // Continue sending the file
        i32 sent = send_file_chunk(con_state, pcb, state->path, state->offset);
        if (sent <= 0) {
            free(state->path);
            free(state);
        }
        if (sent < 0)
            return tcp_close_client_connection(con_state, pcb, ERR_ABRT);
        else if (sent == 0) {
            tcp_write(pcb, "0\r\n\r\n", strlen("0\r\n\r\n"), 0); // Send a zero-length chunk to indicate the end of the transfer
            tcp_output(pcb);                                     // Flush output buffer
            return tcp_close_client_connection(con_state, pcb, ERR_OK);
        }
        state->offset += sent;
    }

    return ERR_OK;
}

// lwIP callback. Will be called when TCP data is received on the connection.
// This is also where we send data back to the client.
static err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    TCPConnection *con_state = (TCPConnection *)arg;
    if (!p || p->tot_len <= 0)
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    assert(con_state && con_state->pcb == pcb);

    LWIP_DEBUGF(TCP_DEBUG, ("tcp_server_recv %d err %d\n", p->tot_len, err));
    for (struct pbuf *q = p; q != NULL; q = q->next)
        LWIP_DEBUGF(TCP_INPUT_DEBUG, ("in: %.*s\n\n\n", q->len, (char *)q->payload));

    // Transfer the received data from the pbuf to our buffer
    char *received = malloc(p->tot_len + 1);
    if (!received) {
        LWIP_DEBUGF(TCP_DEBUG, ("ERROR: failed to allocate received buffer\n"));
        goto close;
    }
    pbuf_copy_partial(p, (void *)received, p->tot_len, 0);
    received[p->tot_len] = '\0';

    bool res = handle_request(con_state, pcb, received);
    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);
    if (!res)
        goto close;

    if (received)
        free(received);
    return ERR_OK;
close:
    if (received)
        free(received);
    return tcp_close_client_connection(con_state, pcb, err);
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
    con_state->state = NULL;

    // Set up callbacks
    tcp_arg(client_pcb, con_state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2); // https://doc.ecoscentric.com/ref/lwip-api-raw-tcp-poll.html
    tcp_err(client_pcb, tcp_server_err);

    return ERR_OK;
}

bool tcp_server_open(TCPServer *state, ip_addr_t *ip, u16 port) {
    LWIP_DEBUGF(TCP_DEBUG, ("starting server on port %d\n", port));
    // First, create a temporary lwIP protocol control block
    // (the real one is created a few lines down using tcp_listen_with_backlog)
    struct tcp_pcb *tempPcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!tempPcb) {
        LWIP_DEBUGF(TCP_DEBUG, ("ERROR: failed to create PCB\n"));
        return false;
    }
    // Bind it to the TCP port, and start listening
    err_t err = tcp_bind(tempPcb, ip ? ip : IP_ANY_TYPE, port);
    if (err) {
        LWIP_DEBUGF(TCP_DEBUG, ("ERROR: failed to bind PCB\n"));
        return false;
    }
    state->server_pcb = tcp_listen_with_backlog(tempPcb, 1);
    state->ip = *ip;
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

bool tcp_server_close(TCPServer *state) {
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        if (!tcp_close(state->server_pcb) != ERR_OK)
            return false;
        state->server_pcb = NULL;
    }
    return true;
}

#endif // PLATFORM_SUPPORTS_WIFI
