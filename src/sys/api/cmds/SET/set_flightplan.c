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

#include "set_flightplan.h"

// Input:
// {"flightplan":<SEE FLIGHTPLAN.C FOR SCHEMA>,"name":""}

/**
 * Helper to parse command arguments.
 * @param args command arguments
 * @param flightplan pointer to store flightplan string
 * @param name pointer to store flightplan name
 * @return true if parsing was successful
 * @note The caller is responsible for freeing the memory allocated for flightplan and name.
 */
static bool parse_args(const char *args, char **flightplan, char **name) {
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
    const char *n = json_object_get_string(obj, "name");
    if (!n) {
        free(*flightplan);
        json_value_free(root);
        return false;
    }
    *name = strdup(n);
    json_value_free(root);
    return true;
}

i32 api_set_flightplan(const char *in, char **out) {
    if (!in) {
        return 400;
    }
    char *flightplan, *name;
    if (!parse_args(in, &flightplan, &name)) {
        return 400;
    }
    bool saved = flightplan_save_json(name, flightplan);
    free(flightplan);
    free(name);
    return saved ? 200 : 500;
    (void)out;
}
