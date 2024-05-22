/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "lib/parson.h"

#include "modes/aircraft.h"

#include "sys/print.h"

#include "get_mode.h"

// {"mode":number}

i32 api_get_mode(const char *args) {
    JSON_Value *root = json_value_init_object();
    JSON_Object *obj = json_value_get_object(root);
    json_object_set_number(obj, "mode", aircraft.mode);
    char *serialized = json_serialize_to_string(root);
    printraw("%s\n", serialized);
    json_free_serialized_string(serialized);
    json_value_free(root);
    return -1;
    (void)args;
}
