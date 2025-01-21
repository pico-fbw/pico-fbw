/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include <string.h>

#include "sys/flightplan.h"

#include "get_flightplan.h"

i32 api_get_flightplan(const char *in, char **out) {
    if (!flightplan_was_parsed()) {
        return 204;
    }
    *out = strdup(flightplan_get()->json);
    return 200;
    (void)in;
}
