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
#include "platform/flash.h"

#include "lwip/debug.h"
#include "lwip/err.h"

#define HTTP_PORT 80

// clang-format on

// Fetches the content requested by a GET request from littlefs and stores it in the result buffer.
// Will be called by the TCP server when a GET request is received.
static int handle_get(const char *request, char **result) {
    // Get the first part of the request (before a space), this is the path to the file being requested
    // Find the first space in the request
    char *end = strchr(request, ' ');
    if (end != NULL)
        *end = '\0'; // End the string at the first space

    // Allocate a buffer to store the path to the file
    // Ensure the buffer is large enough to store anything that could be appended to the path
    char *path = malloc(strlen(request) + sizeof("/www") + sizeof("index.html") + sizeof(".gz"));
    if (path == NULL)
        return ERR_MEM;

    // Prepend "/www" to the path
    strcpy(path, "/www");
    strcat(path, request);
    // If the request is for the root path, append "index.html" to the path
    if (strcmp(request, "/") == 0)
        strcat(path, "index.html");

    // Append .gz to the path
    strcat(path, ".gz");
    // TODO: auto check for .gz vs non-gz (also choose the the right header based on this)
    // also check header to see if client accepts gzip, if not just send an error as we only keep gzipped files

    // Check littlefs for the file
    lfs_file_t file;
    i32 len = 0;
    i32 err = lfs_file_open(&lfs, &file, path, LFS_O_RDONLY);
    LWIP_DEBUGF(TCP_DEBUG, ("GET %s (%ld)\n", path, err));
    // If the file was found, read it into the result buffer
    if (err == LFS_ERR_OK) {
        // Read the contents of the file into the result buffer
        struct lfs_info info;
        lfs_stat(&lfs, path, &info);
        *result = malloc(info.size);
        if (*result == NULL) {
            lfs_file_close(&lfs, &file);
            free(path);
            return ERR_MEM;
        }
        // Read the file into the result buffer
        len = lfs_file_read(&lfs, &file, *result, info.size);
        if (len <= 0) {
            lfs_file_close(&lfs, &file);
            free(path);
            free(*result);
            return len; // littlefs error
        }
        lfs_file_close(&lfs, &file);
    }
    free(path);
    return len;
}

// Handles a POST request and returns the response in the result buffer.
// Will be called by the TCP server when a POST request is received.
static int handle_post(const char *request, char **result) {
    // Extract the request body
    char *body = strstr(request, "\r\n\r\n");
    if (body) {
        body += (sizeof("\r\n\r\n") - 1);
    } else
        return ERR_ARG; // No body, request may be truncated?

    // Extract the endpoint
    char *end = strchr(request, ' ');
    if (end != NULL)
        *end = '\0';
    // Ensure the endpoint begins with "/api/v1/"
    if (strncmp(request, "/api/v1/", sizeof("/api/v1/") - 1) != 0)
        goto invalid;

    // Filter based on the endpoint
    // TODO: improve this and factor out into other functions, this is just a proof of concept
    if (strcmp(request, "/api/v1/ping") == 0) {
    #define PING_RESPONSE "{\"status\":\"OK\"}"
        char *response = malloc(sizeof(PING_RESPONSE));
        if (response == NULL)
            return ERR_MEM;
        strcpy(response, PING_RESPONSE);
        *result = response;
        return strlen(response);
    } else if (strcmp(request, "/api/v1/heap") == 0) {
    #define HEAP_RESPONSE "{\"status\":\"OK\",\"heap\":%d,\"free\":%d,\"used\":%d}"
        struct mallinfo mi = mallinfo();
        int intLen = snprintf(NULL, 0, "%d", INT_MAX);
        int respLen = sizeof(HEAP_RESPONSE) - 6 /* number of %d in HEAP_RESPONSE */ + 3 * intLen;
        char *response = malloc(respLen);
        if (response == NULL)
            return ERR_MEM;
        sprintf(response, HEAP_RESPONSE, mi.arena, mi.fordblks, mi.uordblks);
        *result = response;
        return strlen(response);
    }

// No matching endpoint
invalid : {
    #define INVALID_RESPONSE "{\"status\":\"INVALID\"}"
    char *response = malloc(sizeof(INVALID_RESPONSE));
    if (response == NULL)
        return ERR_MEM;
    strcpy(response, INVALID_RESPONSE);
    *result = response;
    return strlen(response);
}
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
