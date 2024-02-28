/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include "platform/sys.h"
#include "platform/time.h"

#include "io/aahrs.h"

#include "sys/print.h"
#include "sys/runtime.h"

#include "test_aahrs.h"

/**
 * Waits up to timeout_ms for an IMU axis to move past the breakpoint.
 * @param axis The axis to wait for
 * @param breakpoint The breakpoint to wait for
 * @param timeout_ms The timeout in milliseconds
 * @return true if the axis that moved was equal to axis and its difference was positive, false if not.
 */
static bool waitForAxis(IMUAxis axis, u32 breakpoint, u32 timeout_ms) {
    AAHRS original = aahrs; // Take a snapshot of the current position
    Timestamp timeout = timestamp_in_ms(timeout_ms);
    IMUAxis moved = IMU_AXIS_NONE;
    while (!timestamp_reached(&timeout)) {
        i32 diff_roll = ANGLE_DIFFERENCE((i32)original.roll, (i32)aahrs.roll);
        i32 diff_pitch = ANGLE_DIFFERENCE((i32)original.pitch, (i32)aahrs.pitch);
        i32 diff_yaw = ANGLE_DIFFERENCE((i32)original.yaw, (i32)aahrs.yaw);
        if (abs(diff_roll) > breakpoint) {
            printraw("[test] detected roll axis\n");
            if (diff_roll < 0) {
                printraw("[test] incorrect direction detected!\n");
                return false;
            }
            moved = IMU_AXIS_ROLL;
            break;
        } else if (abs(diff_pitch) > breakpoint) {
            printraw("[test] detected pitch axis\n");
            if (diff_pitch < 0) {
                printraw("[test] incorrect direction detected!\n");
                return false;
            }
            moved = IMU_AXIS_PITCH;
            break;
        } else if (abs(diff_yaw) > breakpoint) {
            printraw("[test] detected yaw axis\n");
            if ((diff_yaw) < 0) {
                printraw("[test] incorrect direction detected!\n");
                return false;
            }
            moved = IMU_AXIS_YAW;
            break;
        }
        aahrs.update();
        sys_periodic();
    }
    return moved == axis;
}

i32 api_test_aahrs(const char *cmd, const char *args) {
    printraw("[test] awaiting right roll...\n");
    if (!waitForAxis(IMU_AXIS_ROLL, 20, 10000))
        return 500;
    printraw("[test] return to center.\n");
    runtime_sleep_ms(1500, false);
    printraw("[test] awaiting pitch up...\n");
    if (!waitForAxis(IMU_AXIS_PITCH, 20, 10000))
        return 500;
    printraw("[test] return to center.\n");
    runtime_sleep_ms(1500, false);
    printraw("[test] awaiting right yaw...\n");
    if (!waitForAxis(IMU_AXIS_YAW, 20, 10000))
        return 500;
    return 200;
    // TODO: test baro when done implementing baro fusion
    (void)cmd; // Suppress unused parameter warnings
    (void)args;
}
