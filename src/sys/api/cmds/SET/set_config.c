/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include <string.h>
#include "pico/types.h"

#include "../../../../io/flash.h"

#include "../../../config.h"

#include "set_config.h"

uint api_set_config(const char *cmd, const char *args) {
    if (args) {
        char section[64];
        char key[64];
        char value[64];
        char params[16];
        if (sscanf(args, "%63s %63s %63s %15s", section, key, value, params) < 3) return 400;
        if (!config_set(section, key, value)) return 400;
        // Save to flash immediately if requested
        if (strncasecmp(params, "-S", 2) == 0) {
            goto save;
        }
    } else {
        goto save; // No args, trigger a save to flash
    }
    return 200;

    save:
        if (!config_validate()) return 400;
        flash_save();
        return 200;
}
