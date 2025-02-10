/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "ctrl/aircraft.h"
#include "lib/parson.h"
#include "sys/flightplan.h"

#include "set_active.h"

// Input:
// {"name":""}

// Output:
// {"error":""}

/**
 * Helper to parse command arguments.
 * @param args command arguments
 * @param name pointer to store the flightplan name
 * @return true if parsing was successful
 * @note The caller is responsible for freeing the memory allocated for name.
 */
static bool parse_args(const char *args, char **name) {
    JSON_Value *root = json_parse_string(args);
    if (!root) {
        return false;
    }
    JSON_Object *obj = json_value_get_object(root);
    if (!obj) {
        json_value_free(root);
        return false;
    }
    const char *n = json_object_get_string(obj, "name");
    if (!n) {
        json_value_free(root);
        return false;
    }
    *name = strdup(n);
    json_value_free(root);
    return name;
}

i32 api_set_active(const char *in, char **out) {
    if (!in) {
        return 400;
    }
    if (aircraft.mode == MODE_AUTO) {
        return 403;
    }
    char *name;
    if (!parse_args(in, &name) || !name) {
        return 400;
    }
    // Parse the flightplan into a struct so we can check for errors/set it as active
    Flightplan flightplan;
    FlightplanState err = flightplan_parse(name, &flightplan, true);
    free(name);

    // Construct an error message (if applicable)
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
        case FLIGHTPLAN_ERR_LOAD:
            res = 404;
            break;
        case FLIGHTPLAN_ERR_PARSE:
        case FLIGHTPLAN_ERR_VERSION:
            res = 400;
            break;
        default:
            res = 500;
            break;
    }
    if (json_object_get_string(obj, "error") == NULL) {
        json_object_set_string(obj, "error", "");
    }

    if (res == 200) {
        flightplan_set_active(flightplan);
    }
    char *serialized = json_serialize_to_string(root);
    json_value_free(root);
    *out = serialized;
    return res;
}
