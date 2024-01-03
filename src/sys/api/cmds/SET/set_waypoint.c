/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>

#include "../../../../modes/aircraft.h"
#include "../../../../modes/auto.h"

#include "../../../../wifly/wifly.h"

#include "set_waypoint.h"

void interceptCallback() {
    printf("pico-fbw 200 [WPTINTC]\n");
}

int api_set_waypoint(const char *cmd, const char *args) {
    if (aircraft.mode == MODE_AUTO) {
        Waypoint wpt;
        int numArgs = sscanf(args, "%f %f %d %f %d", &wpt.lat, &wpt.lng, &wpt.alt, &wpt.speed, &wpt.drop);
        switch (numArgs) {
            case 2:
                wpt.alt = -5;
            case 3:
                wpt.speed = -5;
            case 4:
                wpt.drop = 0;
            case 5:
                auto_set(wpt, interceptCallback);
                return 202;
            default:
                return 400;
        }
    } else return 403;
}
