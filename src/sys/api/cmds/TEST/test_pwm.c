/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <math.h>
#include <stdio.h>
#include "platform/time.h"

#include "io/receiver.h"
#include "io/servo.h"

#include "modes/aircraft.h"

#include "sys/configuration.h"
#include "sys/print.h"

#include "test_pwm.h"

i32 api_test_pwm(const char *cmd, const char *args) {
    if (aircraft.mode == MODE_DIRECT) {
        u32 in[] = {config.pins[PINS_INPUT_AIL], config.pins[PINS_INPUT_ELE], config.pins[PINS_INPUT_RUD],
                    config.pins[PINS_INPUT_THROTTLE], config.pins[PINS_INPUT_SWITCH]};
        u32 out[] = {config.pins[PINS_SERVO_AIL], config.pins[PINS_SERVO_ELE], config.pins[PINS_SERVO_RUD],
                     config.pins[PINS_ESC_THROTTLE], config.pins[PINS_SERVO_BAY]};
        u32 numBridges = 5;
        // Use bridges specified by command if any, if not the defaults will be kept
        if (args) {
            i32 numArgs = sscanf(args, "%d %d %d %d %d %d %d %d %d %d", &in[0], &out[0], &in[1], &out[1], &in[2], &out[2],
                                 &in[3], &out[3], &in[4], &out[4]);
            if (numArgs < 2 || numArgs % 2 != 0)
                return 400;
            numBridges = numArgs / 2;
        }
        const u16 testDegrees[] = {23, 67, 82, 153, 169};
        // For every bridge, set the degree value from the predefined set and compare the read value
        for (u32 i = 0; i < numBridges; i++) {
            printraw("[test] testing pin combo %d:%d\n", in[i], out[i]);
            u16 deg = testDegrees[i % (sizeof(testDegrees) / sizeof(testDegrees[0]))];
            servo_set(out[i], deg);
            sleep_ms_blocking(100);
            float degRead = receiver_get(in[i], RECEIVER_MODE_DEG);
            if (fabsf(deg - degRead) > config.control[CONTROL_DEADBAND]) {
                printraw("[test] failed! read %f, expected %d\n", degRead, deg);
                return 500;
            }
        }
    } else
        return 403;
    return 200;
}
