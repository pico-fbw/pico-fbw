/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <stdbool.h>
#include <stdlib.h>
#include "platform/sys.h"
#include "platform/time.h"

#include "ctrl/aircraft.h"
#include "io/imu.h"
#include "sys/print.h"
#include "sys/runtime.h"

#include "test_imu.h"

/**
 * Waits up to timeout_ms for an IMU axis to move past the breakpoint.
 * @param axis The axis to wait for
 * @param breakpoint The breakpoint to wait for
 * @param timeout_ms The timeout in milliseconds
 * @return true if the axis that moved was equal to axis and its difference was positive, false if not.
 */
static bool wait_for_axis(IMUAxis axis, u32 breakpoint, u32 timeout_ms) {
    IMU original = imu; // Take a snapshot of the current position
    Timestamp timeout = timestamp_in_ms(timeout_ms);
    IMUAxis moved = IMU_AXIS_NONE;
    while (!timestamp_reached(&timeout)) {
        i32 diff_roll = ANGLE_DIFFERENCE((i32)original.roll, (i32)imu.roll);
        i32 diff_pitch = ANGLE_DIFFERENCE((i32)original.pitch, (i32)imu.pitch);
        i32 diff_yaw = ANGLE_DIFFERENCE((i32)original.yaw, (i32)imu.yaw);
        if ((u32)abs(diff_roll) > breakpoint) {
            printpre("test", "detected roll axis");
            if (diff_roll < 0) {
                printpre("test", "incorrect direction detected!");
                return false;
            }
            moved = IMU_AXIS_ROLL;
            break;
        } else if ((u32)abs(diff_pitch) > breakpoint) {
            printpre("test", "detected pitch axis");
            if (diff_pitch < 0) {
                printpre("test", "incorrect direction detected!");
                return false;
            }
            moved = IMU_AXIS_PITCH;
            break;
        } else if ((u32)abs(diff_yaw) > breakpoint) {
            printpre("test", "detected yaw axis");
            if ((diff_yaw) < 0) {
                printpre("test", "incorrect direction detected!");
                return false;
            }
            moved = IMU_AXIS_YAW;
            break;
        }
        imu.update();
        sys_periodic();
    }
    return moved == axis;
}

i32 api_test_imu(const char *args) {
    if (aircraft_is_imu_safe()) {
        return 500;
    }
    printpre("test", "awaiting right roll...");
    if (!wait_for_axis(IMU_AXIS_ROLL, 20, 10000)) {
        return 500;
    }
    printpre("test", "return to center.");
    runtime_sleep_ms(1500, false);
    printpre("test", "awaiting pitch up...");
    if (!wait_for_axis(IMU_AXIS_PITCH, 20, 10000)) {
        return 500;
    }
    printpre("test", "return to center.\n");
    runtime_sleep_ms(1500, false);
    printpre("test", "awaiting right yaw...");
    if (!wait_for_axis(IMU_AXIS_YAW, 20, 10000)) {
        return 500;
    }
    return 200;
    // TODO: test baro when done implementing baro fusion
    (void)args;
}
