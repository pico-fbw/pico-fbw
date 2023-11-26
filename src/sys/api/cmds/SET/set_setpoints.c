/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdio.h>
#include "pico/types.h"

#include "../../../../io/flash.h"

#include "../../../../modes/modes.h"
#include "../../../../modes/normal.h"

#include "set_setpoints.h"

uint api_set_setpoints(const char *cmd, const char *args) {
    if (aircraft.mode == MODE_NORMAL) {
        float roll, pitch, yaw;
        if (sscanf(args, "%f %f %f", &roll, &pitch, &yaw) < 3) return 400;
        // Ensure setpoints are within config-defined limits
        if (fabsf(roll) < flash.control[CONTROL_ROLL_LIMIT_HOLD] &&
            pitch < flash.control[CONTROL_PITCH_UPPER_LIMIT] &&
            pitch > flash.control[CONTROL_PITCH_LOWER_LIMIT] &&
            fabsf(yaw) < flash.control[CONTROL_MAX_RUD_DEFLECTION]) {
            // Pass the setpoints into normal mode
            if (mode_normalSetSetpoints(roll, pitch, yaw)) {
                return 200;
            } else return 423; // Rejected by normal mode, likely user inputting which takes priority
        } else return 400;
    } else return 403;
}
