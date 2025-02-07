/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include "ctrl/aircraft.h"

#include "lib/parson.h"

#include "get_mode.h"

// {"mode":"launch|direct|normal|auto|tune|hold"}

i32 api_get_mode(const char *in, char **out) {
    JSON_Value *root = json_value_init_object();
    JSON_Object *obj = json_value_get_object(root);
    json_object_set_string(obj, "mode", mode_to_string(aircraft.mode));
    char *serialized = json_serialize_to_string(root);
    json_value_free(root);
    *out = serialized;
    return 200;
    (void)in;
}
