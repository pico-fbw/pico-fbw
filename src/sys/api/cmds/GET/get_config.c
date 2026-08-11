/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "lib/parson.h"
#include "sys/configuration.h"

#include "get.h"

/**
 * Helper to parse command arguments.
 * @param args command arguments
 * @param section pointer to store the section
 * @param key pointer to store the key
 * @return true if the arguments were parsed successfully
 * @note The caller is responsible for freeing the memory allocated for section and key.
 */
static bool parse_args(const char *args, char **section, char **key) {
    JSON_Value *root = json_parse_string(args);
    if (!root) {
        return false;
    }
    JSON_Object *obj = json_value_get_object(root);
    if (!obj) {
        json_value_free(root);
        return false;
    }
    const char *s = json_object_get_string(obj, "section");
    const char *k = json_object_get_string(obj, "key");
    if (!s || !k) {
        json_value_free(root);
        return false;
    }
    *section = strdup(s);
    *key = strdup(k);
    json_value_free(root);
    return true;
}

/**
 * Helper to get a config value.
 * @param section the section to get the value from
 * @param key the key to get the value of
 * @return the serialized JSON object containing the value
 * @note The caller is responsible for freeing the memory allocated for the output.
 */
static char *get_config_value(const char *section_name, const char *key) {
    void *value = NULL;
    ConfigSectionType type = config_get(section_name, key, &value);
    if (!value || type == SECTION_TYPE_NONE) {
        return NULL;
    }
    // The requested config value exists and we now have it + its type
    // Now, generate our response
    JSON_Value *root = json_value_init_object();
    JSON_Object *obj = json_value_get_object(root);
    JSON_Value *sectionsArr = json_value_init_array();
    JSON_Array *sections = json_value_get_array(sectionsArr);
    JSON_Value *sectionObj = json_value_init_object();
    JSON_Object *section = json_value_get_object(sectionObj);
    json_object_set_string(section, "name", section_name);
    JSON_Value *valuesArr = json_value_init_array();
    JSON_Array *values = json_value_get_array(valuesArr);
    switch (type) {
        case SECTION_TYPE_NUMBER:
            json_array_append_number(values, *(f32 *)value);
            break;
        case SECTION_TYPE_STRING:
            json_array_append_string(values, (char *)value);
            break;
        default:
            json_value_free(root);
            return NULL; // This should never happen
    }
    json_object_set_value(section, "values", valuesArr);
    json_array_append_value(sections, sectionObj);
    json_object_set_value(obj, "sections", sectionsArr);
    char *serialized = json_serialize_to_string(root);
    json_value_free(root);
    return serialized;
}

/**
 * Helper to get the entire config.
 * @return the serialized JSON object containing the entire config
 * @note The caller is responsible for freeing the memory allocated for the output.
 */
static char *get_entire_config() {
    JSON_Value *root = json_value_init_object();
    JSON_Object *obj = json_value_get_object(root);
    JSON_Value *sectionsArr = json_value_init_array();
    JSON_Array *sections = json_value_get_array(sectionsArr);
    for (ConfigSection s = 0; s < CONFIG_SECTION_COUNT; s++) {
        const ConfigSectionInfo *info = config_section_info(s);
        if (!info) {
            json_value_free(root);
            return NULL;
        }
        JSON_Value *sectionObj = json_value_init_object();
        JSON_Object *section = json_value_get_object(sectionObj);
        json_object_set_string(section, "name", info->name);
        JSON_Value *valuesArr = json_value_init_array();
        JSON_Array *values = json_value_get_array(valuesArr);
        for (size_t i = 0; i < info->entryCount; i++) {
            const ConfigEntry *entry = &info->entries[i];
            switch (entry->type) {
                case SECTION_TYPE_NUMBER:
                    json_array_append_number(values, *(f32 *)entry->value);
                    break;
                case SECTION_TYPE_STRING:
                    json_array_append_string(values, (const char *)entry->value);
                    break;
                default:
                    json_value_free(root);
                    return NULL;
            }
        }
        json_object_set_value(section, "values", valuesArr);
        json_array_append_value(sections, sectionObj);
    }
    json_object_set_value(obj, "sections", sectionsArr);
    char *serialized = json_serialize_to_string(root);
    json_value_free(root);
    return serialized;
}

// Input:
// {"section":"","key":""}
// No input will return all config values

// Output:
// {"sections":[{"name":"","values":[number|""]}]}

i32 api_get_config(const char *in, char **out) {
    char *serialized = NULL;
    if (in) {
        // Arguments are present, parse them to figure out what config value to get
        char *section = NULL, *key = NULL;
        if (!parse_args(in, &section, &key)) {
            return 400;
        }
        serialized = get_config_value(section, key);
        free(section);
        free(key);
        if (!serialized) {
            return 400;
        }
    } else {
        // No arguments were given, return all config values
        serialized = get_entire_config();
    }
    *out = serialized;
    return 200;
}
