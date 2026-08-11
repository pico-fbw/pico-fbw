/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "ctrl/aircraft.h"
#include "lib/parson.h"
#include "sys/flightplan.h"

#include "set.h"

// Input:
// {"flightplan":<SEE FLIGHTPLAN.C FOR SCHEMA>|null,"name":""}

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
    if (json_object_has_value(obj, "flightplan")) {
        // Not having a flightplan field is not necessarily an error (unless it's formatted incorrectly)
        // The lack of a field denotes that an existing should be deleted
        JSON_Value *flightplanVal = json_object_get_value(obj, "flightplan");
        if (json_value_get_type(flightplanVal) == JSONObject) {
            *flightplan = json_serialize_to_string(flightplanVal);
            if (!*flightplan) {
                goto fail;
            }
        } else {
            *flightplan = NULL;
        }
    } else {
        *flightplan = NULL;
    }
    const char *name_val = json_object_get_string(obj, "name");
    if (!name_val) {
        goto fail;
    }
    *name = strdup(name_val);
    if (!*name) {
        goto fail;
    }
    json_value_free(root);
    return true;
fail:
    if (*flightplan) {
        json_free_serialized_string(*flightplan);
    }
    json_value_free(root);
    return false;
}

i32 api_set_flightplan(const char *in, char **out) {
    if (!in) {
        return 400;
    }
    char *flightplan, *name;
    if (!parse_args(in, &flightplan, &name)) {
        return 400;
    }
    if (!flightplan) {
        // No flightplan field given, try to delete an existing flightplan
        bool deleted = flightplan_delete(name);
        free(name);
        return deleted ? 200 : 400;
    }
    bool saved = flightplan_save_json(name, flightplan);
    free(flightplan);
    free(name);
    return saved ? 200 : 500;
    (void)out;
}
