/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdlib.h>

#include "modes/aircraft.h"
#include "modes/auto.h"

#include "set_bay.h"

i32 api_set_bay(const char *args) {
    if (aircraft.mode == MODE_NORMAL) {
        BayPosition pos = atoi(args);
        if (pos == CLOSED || pos == OPEN) {
            auto_setBayPosition(pos);
        } else
            return 400;
    } else
        return 403;
    return 200;
}
