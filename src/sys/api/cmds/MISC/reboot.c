/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdlib.h>
#include "platform/sys.h"

#include "reboot.h"

void api_reboot(const char *args) {
    switch (atoi(args)) {
        case 1:
            sys_reboot(true);
        case 0:
        default:
            sys_reboot(false);
    }
}
