/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include <stdbool.h>

#include "ctrl/aircraft.h"
#include "lib/parson.h"
#include "sys/flightplan.h"

#include "set_flightplan.h"

// Input:
// {"flightplan":<SEE FLIGHTPLAN.C FOR SCHEMA>, "active":boolean}

// Output:
// {"error":""}

/**
 * Helper to parse command arguments.
 * @param args command arguments
 * @param flightplan pointer to store flightplan string
 * @param active pointer to store active flag
 * @return true if parsing was successful
 */
static bool parse_args(const char *args, char **flightplan, bool *active) {
    JSON_Value *root = json_parse_string(args);
    if (!root) {
        return false;
    }
    JSON_Object *obj = json_value_get_object(root);
    if (!obj) {
        json_value_free(root);
        return false;
    }
    JSON_Value *flightplanVal = json_object_get_value(obj, "flightplan");
    if (!flightplanVal || json_value_get_type(flightplanVal) != JSONObject) {
        json_value_free(root);
        return false;
    }
    *flightplan = json_serialize_to_string(flightplanVal);
    JSON_Value *activeVal = json_object_get_value(obj, "active");
    if (!activeVal || json_value_get_type(activeVal) != JSONBoolean) {
        json_value_free(root);
        return false;
    }
    *active = json_value_get_boolean(activeVal);
    json_value_free(root);
    return true;
}

i32 api_set_flightplan(const char *in, char **out) {
    if (!in) {
        return 400;
    }
    if (aircraft.mode == MODE_AUTO) {
        return 403;
    }
    char *flightplanStr;
    bool active;
    if (!parse_args(in, &flightplanStr, &active)) {
        return 400;
    }
    // Attempt to parse the flightplan string into a Flightplan struct
    Flightplan flightplan;
    FlightplanState err = flightplan_parse(flightplanStr, &flightplan, true);
    // Construct an error message if applicable
    JSON_Value *root = json_value_init_object();
    if (!root) {
        return 500;
    }
    JSON_Object *obj = json_value_get_object(root);
    if (!obj) {
        json_value_free(root);
        return 500;
    }
    i32 res;
    switch (err) {
        case FLIGHTPLAN_STATUS_OK:
            res = 200;
            break;
        case FLIGHTPLAN_STATUS_GPS_OFFSET:
            json_object_set_string(obj, "error", FLIGHTPLAN_MSG_STATUS_GPS_OFFSET);
            res = 200;
            break;
        case FLIGHTPLAN_WARN_FW_VERSION:
            json_object_set_string(obj, "error", FLIGHTPLAN_MSG_WARN_FW_VERSION);
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
    if (res == 200) {
        if (active) {
            flightplan_set(flightplan);
        }
        // TODO: if not active, save to littlefs
    }
    if (json_object_get_string(obj, "error") == NULL) {
        json_object_set_string(obj, "error", "");
    }
    char *serialized = json_serialize_to_string(root);
    json_value_free(root);
    *out = serialized;
    return res;
}
