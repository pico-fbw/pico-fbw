/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "pico/types.h"

#include "../../../config.h"

#include "get_config.h"

uint api_get_config(const char *cmd, const char *args) {
    if (args) {
        char section[64];
        char key[64];
        if (sscanf(args, "%63s %63s", section, key) < 2) return 400;
        switch (config_getSectionType(section)) {
            case SECTION_TYPE_FLOAT: {
                float k = config_getFloat(section, key);
                if (k != INFINITY) {
                    printf("{\"key\":%f}\n", k);
                } else {
                    return 400;
                }
                break;
            }
            case SECTION_TYPE_STRING: {
                const char *k = config_getString(section, key);
                if (k) {
                    printf("{\"key\":\"%s\"}\n", k);
                } else {
                    return 400;
                }
                break;
            }
        }
    } else {
        // No args, gather all data
        float configFloat[NUM_FLOAT_CONFIG_VALUES];
        config_getAllFloats(configFloat, NUM_FLOAT_CONFIG_VALUES);
        const char *configStr[NUM_STRING_CONFIG_VALUES];
        config_getAllStrings(configStr, NUM_STRING_CONFIG_VALUES);
        printf("{\"sections\":[");
        for (ConfigSection s = 0; s < NUM_CONFIG_SECTIONS; s++) {
            const char *sectionStr = config_sectionToString(s);
            printf("{\"name\":\"%s\",\"keys\":[", sectionStr);
            switch (config_getSectionType(sectionStr)) {
                case SECTION_TYPE_FLOAT: {
                    if (s != NUM_CONFIG_SECTIONS - 1) {
                        for (uint v = 0; v < NUM_FLOAT_VALUES_PER_SECTION; v++) {
                            float value = configFloat[s * NUM_FLOAT_VALUES_PER_SECTION + v];
                            if (v != NUM_FLOAT_VALUES_PER_SECTION - 1) {
                                // For float sectors, check for finite values and change them to null because json is dumb
                                isfinite(value) ? printf("%f,", value) : printf("null,");
                            } else {
                                isfinite(value) ? printf("%f]},", value) : printf("null]},");
                            }
                        }
                    } else {
                        for (uint v = 0; v < NUM_FLOAT_VALUES_PER_SECTION; v++) {
                            float value = configFloat[s * NUM_FLOAT_VALUES_PER_SECTION + v];
                            if (v != NUM_FLOAT_VALUES_PER_SECTION - 1) {
                                isfinite(value) ? printf("%f,", value) : printf("null,");
                            } else {
                                isfinite(value) ? printf("%f]}]}\n", value) : printf("null]}]}\n");
                            }
                        }
                    }
                    break;
                }
                case SECTION_TYPE_STRING: {
                    for (uint v = 0; v < NUM_STRING_VALUES_PER_SECTION; v++) {
                        const char *value = configStr[v];
                        if (v != NUM_STRING_VALUES_PER_SECTION - 1) {
                            value ? printf("\"%s\",", value) : printf("\"\",");
                        } else {
                            value ? printf("\"%s\"]}]}\n", value) : printf("\"\"]}]}\n");
                        }
                    }
                }
            }
        }
    }
    return -1;
}
