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
    if (aircraft.getMode() == MODE_NORMAL) {
        float roll, pitch, yaw;
        if ((sscanf(args, "%f %f %f", &roll, &pitch, &yaw) >= 3) &&
        fabsf(roll) > flash.control[CONTROL_ROLL_LIMIT_HOLD] &&
        pitch < flash.control[CONTROL_PITCH_UPPER_LIMIT] &&
        pitch > flash.control[CONTROL_PITCH_LOWER_LIMIT] &&
        fabsf(yaw) > flash.control[CONTROL_MAX_RUD_DEFLECTION]) {
            if (mode_normalSetSetpoints(roll, pitch, yaw)) {
                return 200;
            } else {
                return 423; // Rejected by normal mode, user probably inputting already
            }
        }
    }
    return 403;
}
