/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lib/parson.h"
#include "sys/configuration.h"

#include "set.h"

// Input:
// {"changes":[{"section":"","key":"","value":""}, ...], "save":boolean}

// Output:
// {"error":""}

// For example:
// {"changes":[{"section":"GENERAL","key":"skipCalibration","value":"1"},{"section":"SYSTEM","key":"ssid","value":"coolwifiname"}],"save":true}
// will set the skipCalibration key in the GENERAL section to 1 (true) and the ssid key in the WIFI section to
// "coolwifiname", and save the changes to flash

i32 api_set_config(const char *in, char **out) {
    JSON_Value *root = json_parse_string(in);
    if (!root) {
        return 400;
    }
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
    // Back up the current config in case of validation failure
    config_backup();
    bool valid = false;
    char error[128] = "";
    // There may be multiple config changes in one request
    for (u32 i = 0; i < json_array_get_count(arr); i++) {
        // For each config change, get the requested config section, key, and new value
        JSON_Object *obj = json_array_get_object(arr, i);
        const char *section = json_object_get_string(obj, "section");
        const char *key = json_object_get_string(obj, "key");
        const char *value = json_object_get_string(obj, "value");
        if (!section || !key || !value) {
            json_value_free(root);
            return 400;
        }
        ConfigSetResult res = config_set(section, key, value);
        if (res == CONFIG_SET_DOES_NOT_EXIST) {
            snprintf(error, sizeof(error), "Section '%s' or key '%s' does not exist.", section, key);
            goto err;
        }
    }
    json_value_free(root);
    // Validate to obtain any errors made in config and save if requested
    valid = config_validate(error, sizeof(error));
    if (save && valid) {
        config_save();
    }
err: {
    bool wasError = strlen(error) > 0;
    if (wasError) {
        // Validation failure, restore the prior config
        config_restore();
    }
    // Make the error JSON even if there was no error (empty string)
    root = json_value_init_object();
    obj = json_value_get_object(root);
    json_object_set_string(obj, "error", error);
    char *serialized = json_serialize_to_string(root);
    json_value_free(root);
    *out = serialized;
}
    return 200;
}
