/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "sys/flightplan.h"
#include "sys/print.h"

#include "get_flightplan.h"

i32 api_get_flightplan(const char *args) {
    if (flightplan_was_parsed()) {
        printraw("%s\n", flightplan_get()->json);
        return -1;
    } else
        return 403;
    (void)args;
}
