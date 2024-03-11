/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <math.h>
#include <stdio.h>

#include "modes/aircraft.h"
#include "modes/normal.h"

#include "sys/configuration.h"

#include "set_target.h"

i32 api_set_target(const char *args) {
    if (aircraft.mode == MODE_NORMAL) {
        float roll, pitch, yaw, throttle;
        i32 numArgs = sscanf(args, "%f %f %f %f", &roll, &pitch, &yaw, &throttle);
        if (numArgs < 3)
            return 400;
        bool hasThrottle = (numArgs >= 4);
        // Ensure setpoints are within config-defined limits
        if (fabsf(roll) > config.control[CONTROL_ROLL_LIMIT_HOLD] || pitch > config.control[CONTROL_PITCH_UPPER_LIMIT] ||
            pitch < config.control[CONTROL_PITCH_LOWER_LIMIT] || fabsf(yaw) > config.control[CONTROL_MAX_RUD_DEFLECTION])
            return 400;
        // Pass the setpoints into normal mode, 423 will be returned if the mode rejects the code (user input takes priority)
        return normal_set(roll, pitch, yaw, throttle, hasThrottle) ? 200 : 423;
    } else
        return 403;
}
