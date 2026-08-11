/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "io/receiver.h"

#include "lib/parson.h"
#include "sys/configuration.h"

#include "get.h"

// {"ail":number,"ele":number,"switch":number,"rud":number,"thr":number}
// Only "ail", "ele", and "switch" are guaranteed to be present

i32 api_get_input(const char *in, char **out) {
    JSON_Value *root = json_value_init_object();
    JSON_Object *obj = json_value_get_object(root);
    json_object_set_number(obj, "ail", receiver_get((i16)config.pins.inputAil, RECEIVER_MODE_DEGREE));
    json_object_set_number(obj, "ele", receiver_get((i16)config.pins.inputEle, RECEIVER_MODE_DEGREE));
    json_object_set_number(obj, "switch", receiver_get((i16)config.pins.inputSwitch, RECEIVER_MODE_DEGREE));
    if (receiver_has_rud()) {
        json_object_set_number(obj, "rud", receiver_get((i16)config.pins.inputRud, RECEIVER_MODE_DEGREE));
    }
    if (receiver_has_athr()) {
        json_object_set_number(obj, "thr", receiver_get((i16)config.pins.inputThrottle, RECEIVER_MODE_PERCENT));
    }
    char *serialized = json_serialize_to_string(root);
    json_value_free(root);
    *out = serialized;
    return 200;
    (void)in;
}
