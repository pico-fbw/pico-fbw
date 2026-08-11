/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "lib/parson.h"
#include "sys/flightplan.h"

#include "get.h"

// Input:
// {"name":""}
// No input will return a list of all flightplan names

// Output:
// <SEE FLIGHTPLAN.C FOR SCHEMA>
// Output (no input):
// {"flightplans":[{"name":"","size":number},...],"active":""|null}

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

i32 api_get_flightplan(const char *in, char **out) {
    if (in) {
        // Attempt to get/return the requested flightplan
        char *name;
        if (!parse_args(in, &name) || !name) {
            return 400;
        }
        char *json = flightplan_get_json(name);
        free(name);
        if (!json) {
            return 404;
        }
        *out = json;
    } else {
        // Didn't request a specific flightplan, return a list of all flightplans
        FlightplanEntry *plans;
        i32 count = flightplan_list(&plans);
        if (count < 0) {
            return 500;
        }
        JSON_Value *root = json_value_init_object();
        if (!root) {
            return 500;
        }
        JSON_Object *obj = json_value_get_object(root);
        if (!obj) {
            json_value_free(root);
            return 500;
        }
        JSON_Value *flightplansArr = json_value_init_array();
        JSON_Array *flightplans = json_value_get_array(flightplansArr);
        for (i32 i = 0; i < count; i++) {
            // Construct an object containing the flightplan name and size
            JSON_Value *entry = json_value_init_object();
            JSON_Object *entryObj = json_value_get_object(entry);
            json_object_set_string(entryObj, "name", plans[i].name);
            json_object_set_number(entryObj, "size", plans[i].size);
            json_array_append_value(flightplans, entry);
        }
        json_object_set_value(obj, "flightplans", flightplansArr);
        // If there's an active flightplan, include it in the response
        Flightplan *active = flightplan_get_active();
        if (active) {
            json_object_set_string(obj, "active", active->name);
        } else {
            json_object_set_null(obj, "active");
        }
        char *serialized = json_serialize_to_string(root);
        json_value_free(root);
        *out = serialized;
    }
    return 200;
}
