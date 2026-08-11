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
// {"sections":[{"name":"","values":[number|""]}, ...], "save":boolean}
// (same schema as GET_CONFIG command; must be a full configuration JSON object)
// Output:
// {"error":""}

i32 api_set_config_full(const char *in, char **out) {
    if (!in) {
        return 400;
    }
    JSON_Value *root = json_parse_string(in);
    if (!root) {
        return 400;
    }
    JSON_Object *obj = json_value_get_object(root);
    if (!obj) {
        json_value_free(root);
        return 400;
    }
    JSON_Array *sections = json_object_get_array(obj, "sections");
    if (!sections) {
        json_value_free(root);
        return 400;
    }
    JSON_Value *saveVal = json_object_get_value(obj, "save");
    if (!saveVal || json_value_get_type(saveVal) != JSONBoolean) {
        json_value_free(root);
        return 400;
    }
    bool save = json_value_get_boolean(saveVal);

    // Backup current config in case of errors
    config_backup();
    char error[128] = "";

    // Iterate through sections
    for (u32 i = 0; i < json_array_get_count(sections); i++) {
        JSON_Object *secObj = json_array_get_object(sections, i);
        if (!secObj) {
            snprintf(error, sizeof(error), "Malformed section object");
            goto err;
        }
        const char *sectionName = json_object_get_string(secObj, "name");
        JSON_Array *values = json_object_get_array(secObj, "values");
        if (!sectionName || !values) {
            snprintf(error, sizeof(error), "Malformed section entry");
            goto err;
        }

        // Find section info by name
        const ConfigSectionInfo *info = NULL;
        ConfigSection s;
        for (s = 0; s < CONFIG_SECTION_COUNT; s++) {
            const ConfigSectionInfo *ci = config_section_info(s);
            if (ci && strcmp(ci->name, sectionName) == 0) {
                info = ci;
                break;
            }
        }
        if (!info) {
            snprintf(error, sizeof(error), "Unknown section '%s'", sectionName);
            goto err;
        }

        if (json_array_get_count(values) != info->entryCount) {
            snprintf(error, sizeof(error), "Incorrect value count for section '%s'", sectionName);
            goto err;
        }

        for (size_t j = 0; j < info->entryCount; j++) {
            const ConfigEntry *entry = &info->entries[j];
            // Serialize the JSON value to text and pass to config_set
            JSON_Value *v = json_array_get_value(values, j);
            char *serialized = json_serialize_to_string(v);
            if (!serialized) {
                snprintf(error, sizeof(error), "Failed to serialize value for '%s' in section '%s'", entry->key,
                         sectionName);
                goto err;
            }

            ConfigSetResult res = config_set(sectionName, entry->key, serialized);
            json_free_serialized_string(serialized);
            if (res == CONFIG_SET_DOES_NOT_EXIST) {
                snprintf(error, sizeof(error), "Section '%s' or key '%s' does not exist.", sectionName, entry->key);
                goto err;
            } // Ignore CONFIG_SET_INVALID return value; validation will catch any errors
        }
    }

    // Validate and optionally save
    bool valid = config_validate(error, sizeof(error));
    if (save && valid) {
        config_save();
    }

err: {
    bool wasError = strlen(error) > 0;
    if (wasError) {
        config_restore();
    }
    JSON_Value *outRoot = json_value_init_object();
    JSON_Object *outObj = json_value_get_object(outRoot);
    json_object_set_string(outObj, "error", error);
    char *serialized = json_serialize_to_string(outRoot);
    json_value_free(outRoot);
    json_value_free(root);
    *out = serialized;
    return 200;
}
}
