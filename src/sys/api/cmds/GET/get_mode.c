/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include "pico/types.h"

#include "../../../../modes/modes.h"

#include "get_mode.h"

uint api_get_mode(const char *cmd, const char *args) {
    printf("{\"mode\":%d}\n", getCurrentMode());
    return -1;
}
