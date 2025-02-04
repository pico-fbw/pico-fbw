/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include "ctrl/aircraft.h"

#include "lib/parson.h"

#include "get_mode.h"

/**
 * @param mode mode to convert to string
 * @return string representation of the mode
 */
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
