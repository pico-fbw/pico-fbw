/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "sys/configuration.h"
#include "sys/print.h"

#include "get_config.h"

/**
 * @param section The section to get the memory offset of
 * @return The memory offset of the section.
 */
static float *getSectionMem(ConfigSection section) {
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

i32 api_get_config(const char *args) {
    if (args) {
        char section[64];
        char key[64];
        if (sscanf(args, "%63s %63s", section, key) < 2)
            return 400;
        void *value = NULL;
        ConfigSectionType type = config_get(section, key, &value);
        if (!value)
            return 400;
        switch (type) {
            case SECTION_TYPE_FLOAT:
                printraw("{\"key\":%f}\n", *(float *)value);
                break;
            case SECTION_TYPE_STRING:
                printraw("{\"key\":\"%s\"}\n", (char *)value);
                break;
            default:
                return 400;
        }
    } else {
        // No args, gather all data
        printraw("{\"sections\":[");
        for (ConfigSection s = 0; s < NUM_CONFIG_SECTIONS; s++) {
            // Section header, based on name
            const char *sectionStr;
            ConfigSectionType type = config_sectionToString(s, &sectionStr);
            printraw("{\"name\":\"%s\",\"keys\":[", sectionStr);
            switch (type) {
                case SECTION_TYPE_FLOAT: {
                    float *section = getSectionMem(s);
                    if (!section)
                        return 400;
                    for (u32 v = 0; v < CONFIG_SECTION_SIZE; v++) {
                        // Read values, up until we hit the end of the data (signified by FLAG_END) or end of the sector
                        if (section[v + 1] != CONFIG_END_MAGIC && v < CONFIG_SECTION_SIZE - 1) {
                            // For float sectors we need to check for finite values and change them to null because json is dumb
                            isfinite(section[v]) ? printraw("%f,", section[v]) : printraw("null,");
                        } else {
                            if (s < NUM_CONFIG_SECTIONS - 1) {
                                isfinite(section[v]) ? printraw("%f]},", section[v]) : printraw("null]},");
                            } else {
                                isfinite(section[v]) ? printraw("%f]}]}\n", section[v]) : printraw("null]}]}\n");
                            }
                            break;
                        }
                    }
                    break;
                }
                case SECTION_TYPE_STRING: {
                    // I didn't feel like figuring out how to loop this, plus there's not much anyway so it's probably smaller
                    // ._.
                    switch (s) {
                        case CONFIG_WIFI:
                            if (s < NUM_CONFIG_SECTIONS - 1) {
                                printraw("\"%s\",\"%s\"]},", config.ssid, config.pass);
                            } else {
                                printraw("\"%s\",\"%s\"]}]}\n", config.ssid, config.pass);
                            }
                        default:
                            break;
                    }
                    break;
                }
                default: {
                    return 500;
                }
            }
        }
    }
    return -1;
}
