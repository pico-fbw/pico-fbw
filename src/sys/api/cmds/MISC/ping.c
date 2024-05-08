/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "sys/print.h"

#include "ping.h"

// I know, this file is crazy, you can thank me later :)

i32 api_ping(const char *args) {
    printraw("PONG\n");
    return -1;
    (void)args;
}
