/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "sys/flightplan.h"

#include "set_flightplan.h"

i32 api_set_flightplan(const char *args) {
    if (!args)
        return 400;
    if (flightplan_was_parsed())
        return 409; // Conflict, a flightplan already exists
    FlightplanError err = flightplan_parse(args, true);
    switch (err) {
        case FLIGHTPLAN_STATUS_OK:
        case FLIGHTPLAN_STATUS_GPS_OFFSET:
        case FLIGHTPLAN_WARN_FW_VERSION:
            return 200;
        case FLIGHTPLAN_ERR_PARSE:
        case FLIGHTPLAN_ERR_VERSION:
            return 400;
        default:
            return 500;
    }
}
