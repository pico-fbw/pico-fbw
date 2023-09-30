/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include <stdlib.h>
#include "pico/types.h"

#include "../../../../io/platform.h"

#include "reboot.h"

void api_reboot(const char *cmd, const char *args) {
    switch (atoi(args)) {
        case 1:
            platform_reboot(REBOOT_BOOTLOADER);
        case 0:
        default:
            platform_reboot(REBOOT_FAST);
    }
}
