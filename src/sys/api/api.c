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
            print("pico-fbw 500");
            free(line);
            return 500;
        }

        i32 status = api_exec(cmd, args);
        if (status != -1)
            print("pico-fbw %ld", status);
        free(line);
        return status;
    }
    return 0;
}

i32 api_call(const char *cmd, const char *args, char **result) {
    FILE *oldStdout = stdout;
    char *buf;
    size_t size = 0;
    FILE *stream = open_memstream(&buf, &size);
    if (!stream)
        return 500;
    stdout = stream;
    i32 status = api_exec(cmd, args);
    fflush(stream);
    fclose(stream);
    stdout = oldStdout;
    *result = buf;
    return status;
}
