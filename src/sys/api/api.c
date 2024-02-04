/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pico/time.h"

#include "hardware/gpio.h"

#include "io/flash.h"
#include "io/serial.h"

#include "sys/api/cmds/cmds.h"

#include "sys/api/api.h"

int api_poll() {
    char *line = stdin_read_line();
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
            printf("pico-fbw 500\n");
            free(line);
            return 500;
        }

        // Command handler
        int status;
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
            printf("pico-fbw %d\n", status);
        }
        free(line);
        return status;
    }
    return 0;
}
