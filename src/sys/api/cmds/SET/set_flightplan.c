/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "lib/parson.h"

#include "sys/flightplan.h"
#include "sys/print.h"

#include "set_flightplan.h"

i32 api_handle_set_flightplan(const char *input, char **output) {
    if (!input)
        return 400;
    JSON_Value *root = json_value_init_object();
    JSON_Object *obj = json_value_get_object(root);
    i32 res;
    FlightplanError err = flightplan_parse(input, true);
    switch (err) {
        case FLIGHTPLAN_STATUS_OK:
            res = 200;
            break;
        case FLIGHTPLAN_STATUS_GPS_OFFSET:
            json_object_set_string(obj, "message", FLIGHTPLAN_MSG_STATUS_GPS_OFFSET);
            res = -1;
            break;
        case FLIGHTPLAN_WARN_FW_VERSION:
            json_object_set_string(obj, "message", FLIGHTPLAN_MSG_WARN_FW_VERSION);
            res = 200;
            break;
        case FLIGHTPLAN_ERR_PARSE:
        case FLIGHTPLAN_ERR_VERSION:
            res = 400;
            break;
        case FLIGHTPLAN_ERR_ALREADY_PARSED:
            res = 409; // Conflict, a flightplan already exists
            break;
        default:
            res = 500;
            break;
    }
    if (json_object_get_string(obj, "message") == NULL)
        json_object_set_string(obj, "message", "");
    char *serialized = json_serialize_to_string(root);
    json_value_free(root);
    *output = serialized;
    return res;
}

// Output:
// {"message":""}

i32 api_set_flightplan(const char *args) {
    char *output = NULL;
    i32 res = api_handle_set_flightplan(args, &output);
    if (!output)
        return 500;
    if (res != 200 && res != -1) {
        json_free_serialized_string(output);
        return res;
    }
    printraw("%s\n", output);
    json_free_serialized_string(output);
    return res;
}
