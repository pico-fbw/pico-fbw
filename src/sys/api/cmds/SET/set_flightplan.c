/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include "ctrl/aircraft.h"
#include "lib/parson.h"
#include "sys/flightplan.h"

#include "set_flightplan.h"

// Output:
// {"message":""}

i32 api_set_flightplan(const char *in, char **out) {
    if (!in) {
        return 400;
    }
    if (aircraft.mode == MODE_AUTO) {
        return 403;
    }
    JSON_Value *root = json_value_init_object();
    JSON_Object *obj = json_value_get_object(root);
    i32 res;
    FlightplanError err = flightplan_parse(in, true);
    switch (err) {
        case FLIGHTPLAN_STATUS_OK:
            res = 200;
            break;
        case FLIGHTPLAN_STATUS_GPS_OFFSET:
            json_object_set_string(obj, "message", FLIGHTPLAN_MSG_STATUS_GPS_OFFSET);
            res = 200;
            break;
        case FLIGHTPLAN_WARN_FW_VERSION:
            json_object_set_string(obj, "message", FLIGHTPLAN_MSG_WARN_FW_VERSION);
            res = 200;
            break;
        case FLIGHTPLAN_ERR_PARSE:
        case FLIGHTPLAN_ERR_VERSION:
            res = 400;
            break;
        default:
            res = 500;
            break;
    }
    if (json_object_get_string(obj, "message") == NULL) {
        json_object_set_string(obj, "message", "");
    }
    char *serialized = json_serialize_to_string(root);
    json_value_free(root);
    *out = serialized;
    return res;
}
