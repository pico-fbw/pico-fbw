/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "pico/time.h"
#include "pico/types.h"

#include "hardware/watchdog.h"

#include "io/aahrs.h"
#include "io/platform.h"

#include "sys/api/cmds/TEST/test_aahrs.h"

/**
 * Waits up to timeout_ms for an IMU axis to move past the breakpoint.
 * @param axis The axis to wait for
 * @param breakpoint The breakpoint to wait for
 * @param timeout_ms The timeout in milliseconds
 * @return true if the axis that moved was equal to axis and its difference was positive, false if not.
*/ 
static bool waitForAxis(IMUAxis axis, uint breakpoint, uint32_t timeout_ms) {
    AAHRS original = aahrs; // Take a snapshot of the current position
    absolute_time_t timeout = make_timeout_time_ms(timeout_ms);
    IMUAxis moved = IMU_AXIS_NONE;
    while (!time_reached(timeout)) {
        int diff_roll = ANGLE_DIFFERENCE((int)original.roll, (int)aahrs.roll);
        int diff_pitch = ANGLE_DIFFERENCE((int)original.pitch, (int)aahrs.pitch);
        int diff_yaw = ANGLE_DIFFERENCE((int)original.yaw, (int)aahrs.yaw);
        if (abs(diff_roll) > breakpoint) {
            printf("[api] detected roll axis\n");
            if (diff_roll < 0) {
                printf("[api] incorrect direction detected!\n");
                return false;
            }
            moved = IMU_AXIS_ROLL;
            break;
        } else if (abs(diff_pitch) > breakpoint) {
            printf("[api] detected pitch axis\n");
            if (diff_pitch < 0) {
                printf("[api] incorrect direction detected!\n");
                return false;
            }
            moved = IMU_AXIS_PITCH;
            break;
        } else if (abs(diff_yaw) > breakpoint) {
            printf("[api] detected yaw axis\n");
            if ((diff_yaw) < 0) {
                printf("[api] incorrect direction detected!\n");
                return false;
            }
            moved = IMU_AXIS_YAW;
            break;
        }
        aahrs.update();
        watchdog_update();
    }
    return moved == axis;
}

int api_test_aahrs(const char *cmd, const char *args) {
    printf("[api] awaiting right roll...\n");
    if (!waitForAxis(IMU_AXIS_ROLL, 20, 10000)) return 500;
    printf("[api] return to center.\n");
    platform_sleep_ms(1500, false);
    printf("[api] awaiting pitch up...\n");
    if (!waitForAxis(IMU_AXIS_PITCH, 20, 10000)) return 500;
    printf("[api] return to center.\n");
    platform_sleep_ms(1500, false);
    printf("[api] awaiting right yaw...\n");
    if (!waitForAxis(IMU_AXIS_YAW, 20, 10000)) return 500;
    return 200;
    // TODO: test baro when done implementing baro fusion
}
