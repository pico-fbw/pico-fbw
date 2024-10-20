/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "lib/parson.h"

#include "sys/configuration.h"
#include "sys/print.h"

#include "get_config.h"

/**
 * @param section The section to get the memory offset of
 * @return The memory offset of the section.
 */
static f32 *get_section_mem(ConfigSection section) {
    switch (section) {
        case CONFIG_GENERAL:
            return config.general;
        case CONFIG_CONTROL:
            return config.control;
        case CONFIG_PINS:
            return config.pins;
        case CONFIG_SENSORS:
            return config.sensors;
        case CONFIG_SYSTEM:
            return config.system;
        default:
            return NULL;
    }
}

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
    if (!root)
        return false;
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
    *section = malloc(strlen(s) + 1);
    *key = malloc(strlen(k) + 1);
    strcpy(*section, s);
    strcpy(*key, k);
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
    if (!value || type == SECTION_TYPE_NONE)
        return NULL;
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
        case SECTION_TYPE_FLOAT:
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
    // For every config section...
    for (ConfigSection s = 0; s < NUM_CONFIG_SECTIONS; s++) {
        JSON_Value *sectionObj = json_value_init_object();
        JSON_Object *section = json_value_get_object(sectionObj);
        const char *sectionStr;
        ConfigSectionType type = config_to_string(s, &sectionStr);
        json_object_set_string(section, "name", sectionStr);
        JSON_Value *valuesArr = json_value_init_array();
        JSON_Array *values = json_value_get_array(valuesArr);
        // ...and for every key in the section, add it to the array
        switch (type) {
            case SECTION_TYPE_FLOAT: {
                f32 *section = get_section_mem(s);
                if (!section)
                    return NULL;
                for (u32 v = 0; v < CONFIG_SECTION_SIZE; v++) {
                    if (section[v + 1] != CONFIG_END_MAGIC && v < CONFIG_SECTION_SIZE - 1) {
                        json_array_append_number(values, section[v]);
                    } else {
                        json_array_append_number(values, section[v]);
                        break;
                    }
                }
                break;
            }
            case SECTION_TYPE_STRING: {
                // I didn't feel like looping this and plus, there's only one string section
                switch (s) {
                    case CONFIG_WIFI:
                        json_array_append_string(values, config.wifi.ssid);
                        json_array_append_string(values, config.wifi.pass);
                        break;
                    default:
                        break;
                }
                break;
            }
            default:
                return NULL; // This should never happen
        }
        json_object_set_value(section, "values", valuesArr);
        json_array_append_value(sections, sectionObj);
    }
    json_object_set_value(obj, "sections", sectionsArr);
    char *serialized = json_serialize_to_string(root);
    json_value_free(root);
    return serialized;
}

i32 api_handle_get_config(const char *input, char **output) {
    char *serialized = NULL;
    if (input) {
        // Arguments are present, parse them to figure out what config value to get
        char *section = NULL, *key = NULL;
        if (!parse_args(input, &section, &key))
            return 400;
        serialized = get_config_value(section, key);
        free(section);
        free(key);
        if (!serialized)
            return 400;
    } else {
        // No arguments were given, return all config values
        serialized = get_entire_config();
    }
    *output = serialized;
    return 200;
}

// Input:
// {"section":"","key":""}

// Output:
// {"sections":[{"name":"","keys":[number|""]}]}

i32 api_get_config(const char *args) {
    char *output = NULL;
    i32 res = api_handle_get_config(args, &output);
    if (res != 200 || !output) {
        if (output)
            json_free_serialized_string(output);
        return res;
    }
    printraw("%s\n", output);
    json_free_serialized_string(output);
    return -1;
}
