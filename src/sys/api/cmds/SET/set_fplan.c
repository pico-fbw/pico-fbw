/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "sys/flightplan.h"

#include "set_fplan.h"

i32 api_set_fplan(const char *cmd, const char *args) {
    if (!args)
        return 400;
    FlightplanError err = flightplan_parse(args);
    switch (err) {
        case FPLAN_STATUS_OK:
        case FPLAN_STATUS_GPS_OFFSET:
        case FPLAN_WARN_FW_VERSION:
            return 200;
        case FPLAN_ERR_PARSE:
        case FPLAN_ERR_VERSION:
            return 400;
        default:
            return 500;
    }
}
