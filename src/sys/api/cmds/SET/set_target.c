/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <math.h>
#include <stdio.h>

#include "io/flash.h"

#include "modes/aircraft.h"
#include "modes/normal.h"

#include "sys/api/cmds/SET/set_target.h"

int api_set_target(const char *cmd, const char *args) {
    if (aircraft.mode == MODE_NORMAL) {
        float roll, pitch, yaw, throttle;
        int numArgs = sscanf(args, "%f %f %f %f", &roll, &pitch, &yaw, &throttle);
        if (numArgs < 3) return 400;
        bool hasThrottle = (numArgs >= 4);
        // Ensure setpoints are within config-defined limits
        if (fabsf(roll) > flash.control[CONTROL_ROLL_LIMIT_HOLD] ||
            pitch > flash.control[CONTROL_PITCH_UPPER_LIMIT] ||
            pitch < flash.control[CONTROL_PITCH_LOWER_LIMIT] ||
            fabsf(yaw) > flash.control[CONTROL_MAX_RUD_DEFLECTION])
            return 400;
        // Pass the setpoints into normal mode, 423 will be returned if the mode rejects the code (user input takes priority)
        return normal_set(roll, pitch, yaw, throttle, hasThrottle) ? 200 : 423;
    } else return 403;
}
