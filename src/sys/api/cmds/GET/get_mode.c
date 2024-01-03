/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>

#include "../../../../modes/aircraft.h"

#include "get_mode.h"

int api_get_mode(const char *cmd, const char *args) {
    printf("{\"mode\":%d}\n", aircraft.mode);
    return -1;
}
