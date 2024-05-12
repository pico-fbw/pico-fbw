/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <string.h>

#include "lib/parson.h"

#include "modes/aircraft.h"
#include "modes/auto.h"

#include "set_bay.h"

/**
 * Helper to parse command arguments.
 * @param args command arguments
 * @return bay position
 */
static BayPosition parse_args(const char *args) {
    JSON_Value *root = json_parse_string(args);
    if (!root)
        return POS_INVALID;
    JSON_Object *obj = json_value_get_object(root);
    if (!obj) {
        json_value_free(root);
        return POS_INVALID;
    }
    const char *posStr = json_object_get_string(obj, "position");
    if (!posStr) {
        json_value_free(root);
        return POS_INVALID;
    }
    BayPosition pos;
    if (strcasecmp(posStr, "closed") == 0)
        pos = POS_CLOSED;
    else if (strcasecmp(posStr, "open") == 0)
        pos = POS_OPEN;
    else {
        json_value_free(root);
        return POS_INVALID;
    }
    json_value_free(root);
    return pos;
}

// {"position":"open|closed"}

i32 api_set_bay(const char *args) {
    if (aircraft.mode != MODE_NORMAL)
        return 403;

    BayPosition position = parse_args(args);
    if (position == POS_INVALID)
        return 400;
    auto_set_bay_position(position);
    return 200;
}
