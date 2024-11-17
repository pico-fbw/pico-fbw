/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include "lib/parson.h"

#include "modes/aircraft.h"

#include "sys/print.h"

#include "get_mode.h"

// {"mode":"launch|direct|normal|auto|tune|hold"}

static const char *mode_to_string(Mode mode) {
    switch (mode) {
        case MODE_LAUNCH:
            return "launch";
        case MODE_DIRECT:
            return "direct";
        case MODE_NORMAL:
            return "normal";
        case MODE_AUTO:
            return "auto";
        case MODE_TUNE:
            return "tune";
        case MODE_HOLD:
            return "hold";
        default:
            return "invalid";
    }
}

i32 api_get_mode(const char *args) {
    JSON_Value *root = json_value_init_object();
    JSON_Object *obj = json_value_get_object(root);
    json_object_set_string(obj, "mode", mode_to_string(aircraft.mode));
    char *serialized = json_serialize_to_string(root);
    printraw("%s\n", serialized);
    json_free_serialized_string(serialized);
    json_value_free(root);
    return -1;
    (void)args;
}
