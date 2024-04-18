/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "http.h"

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/defs.h"
#include "platform/flash.h"

#include "lwip/debug.h"
#include "lwip/err.h"

#define HTTP_PORT 80

#define HTTP_RESPONSE_HEADER "HTTP/1.1 %d OK\nContent-Length: %ld\nContent-Type: %s\nContent-Encoding: gzip\nConnection: close\n\n"
#define HTTP_RESPONSE_HEADER_NOGZIP "HTTP/1.1 %d OK\nContent-Length: %ld\nContent-Type: %s\nConnection: close\n\n"

#define HTTP_BODY_ERR "<!doctype html><html><head><title>pico-fbw</title></head><body><h1>Oops!</h1><p>An error occured whilst loading pico-fbw.<br>%s<br><br><i>pico-fbw v"PICO_FBW_VERSION"</i></p></body></html>"
#define ERR_NOGZIP "Looks like your browser doesn't support gzip compression. Please try again with a newer browser."
#define ERR_RETRIEVE "The requested content could not be retrieved."

// If a content fetch within a GET request fails, the client will be redirected to this location
#define REDIRECT_LOCATION "/"
#define HTTP_RESPONSE_REDIRECT "HTTP/1.1 302 Redirect\nLocation: http://%s" REDIRECT_LOCATION "\n\n"

// clang-format on

// TODO: rewrite http/tcp code (take inspiration from arduino esp32 core and webserver, etc.) to be simpler, send in chunks, etc.

/**
 * Gets the MIME type of a file based on its extension.
 * @param file the file name
 * @return the MIME type of the file
 */
const char *get_mime_type(const char *file) {
    const char *ext = strrchr(file, '.');
    if (ext == NULL)
        return NULL; // No "." in file name
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) {
        return "text/html";
    } else if (strcmp(ext, ".js") == 0) {
        return "application/javascript";
    } else if (strcmp(ext, ".css") == 0) {
        return "text/css";
    } else if (strcmp(ext, ".svg") == 0) {
        return "image/svg+xml";
    } else if (strcmp(ext, ".gif") == 0) {
        return "image/gif";
    } else
        return "application/octet-stream";
}

// Fetches the content requested by a GET request from littlefs and responds with the content.
// Will be called by the TCP server when a GET request is received.
static i32 handle_get(const TCPConnection *connection, const char *request, char **header, i32 *header_len, char **body,
                      i32 *body_len) {
    return -1;
}

// Handles a POST request and responds with the appropriate data.
// Will be called by the TCP server when a POST request is received.
static i32 handle_post(const TCPConnection *connection, const char *request, char **header, i32 *header_len, char **body,
                       i32 *body_len) {
    return -1;
}

bool http_server_init(HTTPServer *server, ip_addr_t *ip) {
    server->ip = *ip;
    server->on_get = handle_get;
    server->on_post = handle_post;
    return tcp_server_open(server, HTTP_PORT);
}

void http_server_close(HTTPServer *server) {
    tcp_server_close(server);
}

#endif // PLATFORM_SUPPORTS_WIFI
