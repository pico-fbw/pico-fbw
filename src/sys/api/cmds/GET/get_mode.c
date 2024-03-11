/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "modes/aircraft.h"

#include "sys/print.h"

#include "get_mode.h"

i32 api_get_mode(const char *args) {
    printraw("{\"mode\":%d}\n", aircraft.mode);
    return -1;
    (void)args;
}
