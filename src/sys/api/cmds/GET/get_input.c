/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "io/receiver.h"

#include "lib/parson.h"

#include "sys/configuration.h"
#include "sys/print.h"

#include "get_input.h"

// {"ail":number,"ele":number,"rud":number,"thr":number,"switch":number}
// Only "ail" and "ele" are guaranteed to be present

i32 api_get_input(const char *args) {
    JSON_Value *root = json_value_init_object();
    JSON_Object *obj = json_value_get_object(root);
    json_object_set_number(obj, "ail", receiver_get(config.pins[PINS_INPUT_AIL], RECEIVER_MODE_DEGREE));
    json_object_set_number(obj, "ele", receiver_get(config.pins[PINS_INPUT_ELE], RECEIVER_MODE_DEGREE));
    switch ((ControlMode)config.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
            json_object_set_number(obj, "thr", receiver_get(config.pins[PINS_INPUT_THROTTLE], RECEIVER_MODE_PERCENT));
        /* fall through */
        case CTRLMODE_3AXIS:
            json_object_set_number(obj, "rud", receiver_get(config.pins[PINS_INPUT_RUD], RECEIVER_MODE_DEGREE));
            json_object_set_number(obj, "switch", receiver_get(config.pins[PINS_INPUT_SWITCH], RECEIVER_MODE_DEGREE));
            break;
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_FLYINGWING_ATHR:
            json_object_set_number(obj, "thr", receiver_get(config.pins[PINS_INPUT_THROTTLE], RECEIVER_MODE_PERCENT));
        /* fall through */
        case CTRLMODE_2AXIS:
        case CTRLMODE_FLYINGWING:
            json_object_set_number(obj, "switch", receiver_get(config.pins[PINS_INPUT_SWITCH], RECEIVER_MODE_DEGREE));
            break;
    }
    char *serialized = json_serialize_to_string(root);
    printraw("%s\n", serialized);
    json_free_serialized_string(serialized);
    json_value_free(root);
    return -1;
    (void)args;
}
