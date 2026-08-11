/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <string.h>

#include "ctrl/aircraft.h"
#include "lib/parson.h"

#include "set.h"

/**
 * Helper to parse command arguments.
 * @param args command arguments
 * @return the parsed mode
 */
static Mode parse_args(const char *args) {
    JSON_Value *root = json_parse_string(args);
    if (!root) {
        return MODE_INVALID;
    }
    JSON_Object *obj = json_value_get_object(root);
    if (!obj) {
        json_value_free(root);
        return MODE_INVALID;
    }
    const char *modeStr = json_object_get_string(obj, "mode");
    if (!modeStr) {
        json_value_free(root);
        return MODE_INVALID;
    }
    Mode mode;
    if (strcasecmp(modeStr, "direct") == 0) {
        mode = MODE_DIRECT;
    } else if (strcasecmp(modeStr, "normal") == 0) {
        mode = MODE_NORMAL;
    } else if (strcasecmp(modeStr, "auto") == 0) {
        mode = MODE_AUTO;
    } else {
        mode = MODE_INVALID;
    }
    json_value_free(root);
    return mode;
}

// {"mode":"direct|normal|auto"}

i32 api_set_mode(const char *in, char **out) {
    Mode newMode = parse_args(in);
    if (newMode == MODE_INVALID) {
        return 400;
    }
    aircraft_change_mode(newMode);
    return 200;
    (void)out;
}
