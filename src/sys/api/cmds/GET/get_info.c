/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/defs.h"

#include "sys/print.h"
#include "sys/version.h"

#include "get_info.h"

i32 api_get_info(const char *cmd, const char *args) {
    printraw("{\"version\":\"%s\",\"version_api\":\"%s\",\"version_fplan\":\"%s\",\"platform\":\"%s\",\"version_platform\":\"%"
             "s\"}\n",
             PICO_FBW_VERSION, PICO_FBW_API_VERSION, FPLAN_VERSION, PLATFORM_NAME, PLATFORM_HAL_VERSION);
    return -1;
}
