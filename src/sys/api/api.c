/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sys/api/cmds/cmds.h"
#include "sys/print.h"

#include "api.h"

/**
 * Executes an API command.
 * @param cmd command to execute
 * @param args arguments to the command
 * @return status code of the command
 */
static i32 api_exec(const char *cmd, const char *args) {
    i32 status;
    if (strncasecmp(cmd, "GET_", 4) == 0) {
        status = api_handle_get(cmd, args);
    } else if (strncasecmp(cmd, "SET_", 4) == 0) {
        status = api_handle_set(cmd, args);
    } else if (strncasecmp(cmd, "TEST_", 5) == 0) {
        status = api_handle_test(cmd, args);
    } else
        status = api_handle_misc(cmd, args);
    return status;
}

i32 api_poll() {
    char *line = stdin_read();
    if (line) {
        if (strlen(line) < 1) {
            free(line);
            return 0;
        }
        // Seperate the command and arguments
        char *cmd = strtok(line, " ");
        char *args = strtok(NULL, "");
        if (!cmd) {
            // Out of memory?
            printraw("pico-fbw 500\n");
            free(line);
            return 500;
        }

        i32 status = api_exec(cmd, args);
        if (status != -1)
            printraw("pico-fbw %ld\n", status);
        free(line);
        return status;
    }
    return 0;
}

const char *api_res_to_http_status(i32 res) {
    switch (res) {
        case -1: // -1 is an alternate API code which more or less means the same as 200
        case 200:
            return "200 OK";
        case 202:
            return "202 Accepted";
        case 204:
            return "204 No Content";
        case 400:
            return "400 Bad Request";
        case 403:
            return "403 Forbidden";
        case 404:
            return "404 Not Found";
        case 409:
            return "409 Conflict";
        case 500:
        default:
            return "500 Internal Server Error";
    }
}
