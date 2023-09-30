/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include "pico/types.h"

#include "../../../../modes/modes.h"
#include "../../../../modes/normal.h"

#include "../../../config.h"

#include "set_setpoints.h"

uint api_set_setpoints(const char *cmd, const char *args) {
    if (getCurrentMode() == MODE_NORMAL) {
        float roll, pitch, yaw;
        if ((sscanf(args, "%f %f %f", &roll, &pitch, &yaw) >= 3) &&
        (roll > config.limits.rollLimitHold || roll < -config.limits.rollLimitHold) &&
        (pitch > config.limits.pitchUpperLimit || pitch < config.limits.pitchLowerLimit) &&
        (yaw > config.limits.maxRudDeflection || yaw < -config.limits.maxRudDeflection)) {
            if (mode_normalSetSetpoints(roll, pitch, yaw)) {
                return 200;
            } else {
                return 423; // Rejected by normal mode, user probably inputting already
            }
        }
    } else {
        return 403;
    }
}
