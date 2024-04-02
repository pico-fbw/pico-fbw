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

// TODO: refactor and fix this code, it's very much a work in progress and also very gross

// Fetches the content requested by a GET request from littlefs and responds with the content.
// Will be called by the TCP server when a GET request is received.
static i32 handle_get(const TCPConnection *connection, const char *request, char **header, i32 *header_len, char **body,
                      i32 *body_len) {
    // Copy the request string since it will be modified to obtain the path
    char *requestCopy = strdup(request);
    if (requestCopy == NULL)
        return ERR_MEM;
    // Get the first part of the request (before a space), this is the path to the file being requested
    // Find the first space in the request
    char *end = strchr(requestCopy, ' ');
    if (end != NULL)
        *end = '\0'; // End the string at the first space

    // Allocate a buffer to store the path to the file
    // Ensure the buffer is large enough to store anything that could be appended to the path
    char *path = malloc(strlen(requestCopy) + sizeof("/www") + sizeof("index.html") + sizeof(".gz"));
    if (path == NULL)
        return ERR_MEM;

    // Prepend "/www" to the path since web assets are stored in "/www/..." in the wwwfs
    strcpy(path, "/www");
    strcat(path, requestCopy);
    // If the request is for the root path, append "index.html"
    bool root = false;
    if (strcmp(requestCopy, "/") == 0) {
        strcat(path, "index.html");
        root = true;
    }

    // Check littlefs for the file
    bool gzipped = false;
    lfs_file_t file;
    i32 len = 0;
    i32 err;
    for (u32 i = 0; i < 2; i++) {
        err = lfs_file_open(&wwwfs, &file, path, LFS_O_RDONLY);
        LWIP_DEBUGF(TCP_DEBUG, ("GET %s (%ld)\n", path, err));
        if (err == LFS_ERR_NOENT && !gzipped) {
            // If the file was not found, try once more with the .gz extension
            strcat(path, ".gz");
            gzipped = true;
            continue;
        }
        break;
    }

    // If the file was found, read it into the result buffer
    if (err == LFS_ERR_OK) {
        // Read the contents of the file into the result buffer
        struct lfs_info info;
        lfs_stat(&wwwfs, path, &info);
        *body = malloc(info.size);
        if (*body == NULL) {
            lfs_file_close(&wwwfs, &file);
            free(path);
            return ERR_MEM;
        }
        // Read the file into the result buffer
        len = lfs_file_read(&wwwfs, &file, *body, info.size);
        if (len <= 0) {
            lfs_file_close(&wwwfs, &file);
            free(path);
            free(*body);
            return len; // littlefs error
        }
        lfs_file_close(&wwwfs, &file);
    }

    // Response body is complete (may or may not be empty), construct the response header
    if (len == 0) {
        // No file found, redirect to the root path
        *header = malloc(sizeof(HTTP_RESPONSE_REDIRECT) + strlen(ipaddr_ntoa(connection->ip)));
        if (!*header) {
            free(path);
            return ERR_MEM;
        }
        sprintf(*header, HTTP_RESPONSE_REDIRECT, ipaddr_ntoa(connection->ip));
    } else {
        // Get the MIME type (basically the content type) of the file
        const char *mime = get_mime_type(requestCopy); // Not path because that may have the .gz extension
        if (root)
            mime = "text/html"; // Root path is index.html; always HTML
        if (mime == NULL) {
            *body = malloc(sizeof(HTTP_BODY_ERR) + sizeof(ERR_RETRIEVE));
            if (*body == NULL) {
                free(path);
                return ERR_MEM;
            }
            len = sprintf(*body, HTTP_BODY_ERR, ERR_RETRIEVE);
            goto noGz;
        }
        // Headers differ based on whether the file is gzipped
        if (gzipped) {
            const char *accept = strstr(request, "Accept-Encoding: gzip");
            if (!accept) {
                // Client does not accept gzip, but we only have a gzipped version
                *body = malloc(sizeof(HTTP_BODY_ERR) + sizeof(ERR_NOGZIP));
                if (*body == NULL) {
                    free(path);
                    return ERR_MEM;
                }
                len = sprintf(*body, HTTP_BODY_ERR, ERR_NOGZIP);
                goto noGz;
            }
            *header = malloc(sizeof(HTTP_RESPONSE_HEADER) + sizeof(int) + sizeof(int) + strlen(mime));
            if (!*header) {
                free(path);
                return ERR_MEM;
            }
            sprintf(*header, HTTP_RESPONSE_HEADER, 200, len, mime);
        } else {
        noGz:
            *header = malloc(sizeof(HTTP_RESPONSE_HEADER) + sizeof(int) + sizeof(int) + strlen(mime));
            if (!*header) {
                free(path);
                return ERR_MEM;
            }
            sprintf(*header, HTTP_RESPONSE_HEADER_NOGZIP, 200, len, mime);
        }
    }
    free(path);
    *header_len = strlen(*header);
    *body_len = len;
    return 0;
}

// Handles a POST request and responds with the appropriate data.
// Will be called by the TCP server when a POST request is received.
static i32 handle_post(const TCPConnection *connection, const char *request, char **header, i32 *header_len, char **body,
                       i32 *body_len) {
    // Extract the request body
    char *reqBody = strstr(request, "\r\n\r\n");
    if (reqBody) {
        reqBody += (sizeof("\r\n\r\n") - 1);
    } else
        goto invalid; // No body, request may be truncated?

    // Extract the (full) endpoint
    char *fullEnd = strchr(request, ' ');
    if (fullEnd != NULL)
        *fullEnd = '\0';
    // Extract the actual endpoint (this also ensures the endpoint begins with "/api/v1/"; a valid endpoint)
    char *end = strstr(request, "/api/v1/");
    if (end == NULL)
        goto invalid;              // Invalid endpoint
    end += sizeof("/api/v1/") - 1; // Skip "/api/v1/"

    // Filter based on the endpoint
    // TODO: improve this and factor out into other functions, this is just a proof of concept
    printf("POST %s\n", end);
    if (strcmp(end, "ping") == 0) {
    #define PING_RESPONSE "{\"err\":0}"
        *body = malloc(sizeof(PING_RESPONSE));
        if (!*body)
            return ERR_MEM;
        strcpy(*body, PING_RESPONSE);
    } else {
    // No matching endpoint
    invalid : {
    #define ERR_ARG "1"
    #define INVALID_RESPONSE "{\"err\":" ERR_ARG "}"
        *body = malloc(sizeof(INVALID_RESPONSE));
        if (!*body)
            return ERR_MEM;
        strcpy(*body, INVALID_RESPONSE);
    }
    }
    *header_len = strlen(*header);

    const char *mime = "application/json";
    *header = malloc(sizeof(HTTP_RESPONSE_HEADER_NOGZIP) + sizeof(int) + sizeof(int) + strlen(mime));
    sprintf(*header, HTTP_RESPONSE_HEADER_NOGZIP, 200, *header_len, mime);
    *body_len = strlen(*body);
    return 0;
    (void)connection;
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
