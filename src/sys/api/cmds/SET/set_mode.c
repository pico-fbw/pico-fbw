/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <string.h>

#include "lib/parson.h"

#include "modes/aircraft.h"

#include "set_mode.h"

/**
 * Helper to parse command arguments.
 * @param args command arguments
 * @return the parsed mode
 */
static Mode parse_args(const char *args) {
    JSON_Value *root = json_parse_string(args);
    if (!root)
        return MODE_INVALID;
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
    if (strcasecmp(modeStr, "direct") == 0)
        mode = MODE_DIRECT;
    else if (strcasecmp(modeStr, "normal") == 0)
        mode = MODE_NORMAL;
    else if (strcasecmp(modeStr, "auto") == 0)
        mode = MODE_AUTO;
    else {
        json_value_free(root);
        return MODE_INVALID;
    }
    json_value_free(root);
    return mode;
}

// {"mode":"direct|normal|auto"}

i32 api_set_mode(const char *args) {
    Mode newMode = parse_args(args);
    if (newMode == MODE_INVALID)
        return 400;
    aircraft.change_to(newMode);
    return 200;
}
