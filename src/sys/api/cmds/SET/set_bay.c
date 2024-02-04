/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <stdio.h>
#include <stdlib.h>

#include "modes/aircraft.h"
#include "modes/auto.h"

#include "sys/api/cmds/SET/set_bay.h"

int api_set_bay(const char *cmd, const char *args) {
    if (aircraft.mode == MODE_NORMAL) {
        BayPosition pos = atoi(args);
        if (pos >= CLOSED && pos <= OPEN) {
            auto_setBayPosition(pos);
        } else return 400;
    } else return 403;
}
