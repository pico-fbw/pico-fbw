/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <stdio.h>

#include "sys/api/cmds/MISC/ping.h"

// I know, this file is crazy, thank me later :)

void api_ping(const char *cmd, const char *args) {
    printf("PONG\n");
}
