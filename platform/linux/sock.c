/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "platform/defs.h"

#include "sys/api/api.h"

#include "platform/sock.h"

#define SOCK_PATH "/tmp/pico-fbw.sock"
#define SOCK_QUEUELEN 1

static int serverFd = -1; // Server socket file descriptor
static pthread_t sockThread = 0;

/**
 * api_output_func-compatible fprintf wrapper for API output to socket
 * @param ctx file pointer to the socket
 * @param fmt format string
 * @param ... printf-like arguments
 */
static int __printflike(2, 3) sock_printf(void *ctx, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf((FILE *)ctx, fmt, args);
    va_end(args);
    return ret;
}

/**
 * Handle an API client connection.
 * @param fd file descriptor of the client connection
 */
static void handle_client(int fd) {
    FILE *fp = fdopen(fd, "r+");
    if (!fp) {
        close(fd);
        return;
    }
    // Read lines from the client
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, fp)) != -1) {
        // Discard empty lines, remove trailing newline
        if (strlen(line) < 1) {
            free(line);
            continue;
        }
        line[strcspn(line, "\n")] = 0;
        // Parse command and arguments
        char *cmd = strtok(line, " ");
        char *args = strtok(NULL, "");
        i32 status;
        // Handle GET and SET commands, others are not supported (return 404)
        if (strncasecmp(cmd, "GET_", 4) == 0) {
            status = api_handle_get(cmd, args, sock_printf, fp);
        } else if (strncasecmp(cmd, "SET_", 4) == 0) {
            status = api_handle_set(cmd, args, sock_printf, fp);
        } else {
            status = 404;
        }
        if (status != -1) {
            // If output wasn't already written, write the status code
            fprintf(fp, "pico-fbw %d\n", status);
        }
        free(line);
    }
    fclose(fp);
    close(fd);
}

// Thread for handling incoming connections.
static void *sock_thread(void *arg) {
    while (true) {
        int clientFd = accept(serverFd, NULL, NULL);
        if (clientFd != -1) {
            handle_client(clientFd);
        }
    }
    return NULL;
    (void)arg;
}

bool sock_setup() {
    // Create a Unix domain socket
    serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverFd == -1) {
        return false;
    }
    // Bind the socket to the path
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);
    unlink(SOCK_PATH); // Remove any existing socket
    if (bind(serverFd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        close(serverFd);
        return false;
    }
    // Listen for incoming connections on a separate thread
    if (listen(serverFd, SOCK_QUEUELEN) == -1) {
        close(serverFd);
        return false;
    }
    if (pthread_create(&sockThread, NULL, sock_thread, NULL) != 0) {
        close(serverFd);
        return false;
    }
    return true;
}
