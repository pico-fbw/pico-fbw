/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdio.h>

#include "modes/aircraft.h"
#include "modes/auto.h"

#include "sys/print.h"

#include "set_waypoint.h"

void interceptCallback() {
    printraw("pico-fbw 200 [WPTINTC]\n");
}

i32 api_set_waypoint(const char *args) {
    if (aircraft.mode == MODE_AUTO) {
        Waypoint wpt;
        i32 numArgs = sscanf(args, "%lf %lf %ld %f %ld", &wpt.lat, &wpt.lng, &wpt.alt, &wpt.speed, &wpt.drop);
        switch (numArgs) {
            case 2:
                wpt.alt = -5;
            /* fall through */
            case 3:
                wpt.speed = -5;
            /* fall through */
            case 4:
                wpt.drop = 0;
            /* fall through */
            case 5:
                auto_set(wpt, interceptCallback);
                return 202;
            default:
                return 400;
        }
    } else
        return 403;
}
