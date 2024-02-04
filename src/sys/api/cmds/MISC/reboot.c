/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <stdio.h>
#include <stdlib.h>

#include "io/platform.h"

#include "sys/api/cmds/MISC/reboot.h"

void api_reboot(const char *cmd, const char *args) {
    switch (atoi(args)) {
        case 1:
            platform_reboot(REBOOT_BOOTLOADER);
        case 0:
        default:
            platform_reboot(REBOOT_FAST);
    }
}
