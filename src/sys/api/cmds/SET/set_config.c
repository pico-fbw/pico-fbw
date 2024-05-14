/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdbool.h>
#include <string.h>

#include "lib/parson.h"

#include "sys/configuration.h"

#include "set_config.h"

// {"changes":[{"section":"","key":"","value":""}, ...], "save":boolean}

// For example:
// {"changes":[{"section":"GENERAL","key":"skipCalibration","value":"1"},{"section":"WIFI","key":"ssid","value":"coolwifiname"}],"save":true}
// will set the skipCalibration key in the GENERAL section to 1 (true) and the ssid key in the WIFI section to "coolwifiname",
// and save the changes to flash

i32 api_set_config(const char *args) {
    if (!args)
        goto save; // No args, trigger a save to flash

    JSON_Value *root = json_parse_string(args);
    if (!root)
        return 400;
    JSON_Object *obj = json_value_get_object(root);
    if (!obj) {
        json_value_free(root);
        return 400;
    }
    JSON_Array *arr = json_object_get_array(obj, "changes");
    if (!arr) {
        json_value_free(root);
        return 400;
    }
    JSON_Value *saveVal = json_object_get_value(obj, "save");
    if (!saveVal || json_value_get_type(saveVal) != JSONBoolean) {
        json_value_free(root);
        return 400;
    }
    bool save = json_value_get_boolean(saveVal);
    // There may be multiple config changes in one request
    for (u32 i = 0; i < json_array_get_count(arr); i++) {
        // For each config change, get the requested config section, key, and new value
        JSON_Object *obj = json_array_get_object(arr, i);
        const char *section = json_object_get_string(obj, "section");
        const char *key = json_object_get_string(obj, "key");
        const char *value = json_object_get_string(obj, "value");
        if (!section || !key || !value || !config_set(section, key, value)) {
            json_value_free(root);
            return 400;
        }
    }
    json_value_free(root);
    if (save)
        goto save;
    return 200;

save:
    if (!config_validate())
        return 400;
    config_save();
    return 200;
}
