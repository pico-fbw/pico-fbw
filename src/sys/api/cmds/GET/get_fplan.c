/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include "pico/types.h"

#include "../../../../wifly/wifly.h"

#include "get_fplan.h"

uint api_get_fplan(const char *cmd, const char *args) {
    const char *fplan = wifly_getFplanJson();
    if (fplan != NULL && wifly_getWaypointCount() > 0) {
        printf("%s\n", fplan);
        return 200;
    } else {
        return 403;
    }
}
