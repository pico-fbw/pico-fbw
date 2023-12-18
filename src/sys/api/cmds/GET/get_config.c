/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "pico/platform.h"
#include "pico/types.h"

#include "../../../../io/flash.h"

#include "../../../config.h"

#include "get_config.h"

/**
 * @param section The section to get the memory offset of
 * @return The memory offset of the section.
*/
static float *getSectionMem(ConfigSection section) {
    switch (section) {
        case CONFIG_GENERAL:
            return flash.general;
        case CONFIG_CONTROL:
            return flash.control;
        case CONFIG_PINS:
            return flash.pins;
        case CONFIG_SENSORS:
            return flash.sensors;
        case CONFIG_SYSTEM:
            return flash.system;
        case CONFIG_PID:
            return flash.pid;
        default:
            return NULL;
    }
}

int api_get_config(const char *cmd, const char *args) {
    if (args) {
        char section[64];
        char key[64];
        if (sscanf(args, "%63s %63s", section, key) < 2) return 400;
        void *value = NULL;
        ConfigSectionType type = config_get(section, key, &value);
        if (value == NULL) return 400;
        switch (type) {
            case SECTION_TYPE_FLOAT:
                printf("{\"key\":%f}\n", *(float*)value);
                break;
            case SECTION_TYPE_STRING:
                printf("{\"key\":\"%s\"}\n", (char*)value);
                break;
            default:
                return 400;
        }
    } else {
        // No args, gather all data
        printf("{\"sections\":[");
        for (ConfigSection s = 0; s < NUM_CONFIG_SECTIONS; s++) {
            // Section header, based on name
            const char *sectionStr;
            ConfigSectionType type = config_sectionToString(s, &sectionStr);
            printf("{\"name\":\"%s\",\"keys\":[", sectionStr);
            switch (type) {
                case SECTION_TYPE_FLOAT: {
                    float *section = getSectionMem(s);
                    if (section == NULL) return 400;
                    for (uint v = 0; v < FLOAT_SECTOR_SIZE; v++) {
                        // Read values, up until we hit the end of the data (signified by FLAG_END) or end of the sector
                        if (section[v + 1] != FLAG_END && v < FLOAT_SECTOR_SIZE - 1) {
                            // For float sectors we need to check for finite values and change them to null because json is dumb
                            isfinite(section[v]) ? printf("%f,", section[v]) : printf("null,");
                        } else {
                            if (s < NUM_CONFIG_SECTIONS - 1) {
                                isfinite(section[v]) ? printf("%f]},", section[v]) : printf("null]},");
                            } else {
                                isfinite(section[v]) ? printf("%f]}]}\n", section[v]) : printf("null]}]}\n");
                            }
                            break;
                        }
                    }
                    break;
                }
                case SECTION_TYPE_STRING: {
                    // I didn't feel like figuring out how to loop this, plus there's not much anyway so it's probably smaller ._.
                    switch (s) {
                        case CONFIG_WIFLY:
                            if (s < NUM_CONFIG_SECTIONS - 1) {
                                printf("\"%s\",\"%s\"]},", flash.wifly_ssid, flash.wifly_pass);
                            } else {
                                printf("\"%s\",\"%s\"]}]}\n", flash.wifly_ssid, flash.wifly_pass);
                            }
                        default:
                            break;
                    }
                }
            }
        }
    }
    return -1;
}
