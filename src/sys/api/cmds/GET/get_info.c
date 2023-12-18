/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include "pico/config.h"
#include "pico/platform.h"
#include "pico/types.h"

#include "../../../info.h"

#include "get_info.h"

int api_get_info(const char *cmd, const char *args) {
    #if defined(RASPBERRYPI_PICO)
        printf("{\"version\":\"%s\",\"version_api\":\"%s\",\"version_wifly\":\"\",\"is_pico_w\":false,\"rp2040_version\":%d}\n",
            PICO_FBW_VERSION, PICO_FBW_API_VERSION, rp2040_chip_version());
    #elif defined(RASPBERRYPI_PICO_W)
        printf("{\"version\":\"%s\",\"version_api\":\"%s\",\"version_wifly\":\"%s\",\"is_pico_w\":true,\"rp2040_version\":%d}\n",
            PICO_FBW_VERSION, PICO_FBW_API_VERSION, WIFLY_VERSION, rp2040_chip_version());
    #endif
    return -1;
}
