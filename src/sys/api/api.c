/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <string.h>
#include <stdlib.h>

#include "sys/api/cmds/cmds.h"
#include "sys/print.h"

#include "api.h"

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

        // Command handler
        i32 status;
        if (strncasecmp(cmd, "GET_", 4) == 0) {
            status = api_handle_get(cmd, args);
        } else if (strncasecmp(cmd, "SET_", 4) == 0) {
            status = api_handle_set(cmd, args);
        } else if (strncasecmp(cmd, "TEST_", 5) == 0) {
            status = api_handle_test(cmd, args);
        } else {
            status = api_handle_misc(cmd, args);
        }
        if (status != -1) {
            print("pico-fbw %ld", status);
        }
        free(line);
        return status;
    }
    return 0;
}
