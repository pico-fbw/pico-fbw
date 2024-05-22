/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/defs.h"

#include "lib/parson.h"

#include "sys/print.h"
#include "sys/version.h"

#include "get_info.h"

// {"version":"","version_api":"","version_flightplan":"","platform":"","platform_version":""}

i32 api_handle_get_info(char **output) {
    JSON_Value *root = json_value_init_object();
    JSON_Object *obj = json_value_get_object(root);
    json_object_set_string(obj, "version", PICO_FBW_VERSION);
    json_object_set_string(obj, "version_api", PICO_FBW_API_VERSION);
    json_object_set_string(obj, "version_flightplan", FLIGHTPLAN_VERSION);
    json_object_set_string(obj, "platform", PLATFORM_NAME);
    json_object_set_string(obj, "platform_version", PLATFORM_VERSION);
    char *serialized = json_serialize_to_string(root);
    json_value_free(root);
    *output = serialized;
    return 200;
}

i32 api_get_info(const char *args) {
    char *output = NULL;
    i32 res = api_handle_get_info(&output);
    if (!output)
        return 500;
    if (res != 200) {
        json_free_serialized_string(output);
        return res;
    }
    printraw("%s\n", output);
    json_free_serialized_string(output);
    return -1;
    (void)args;
}
