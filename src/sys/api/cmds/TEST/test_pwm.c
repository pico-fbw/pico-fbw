/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdio.h>
#include "pico/rand.h"
#include "pico/time.h"
#include "pico/types.h"

#include "../../../../io/flash.h"
#include "../../../../io/pwm.h"
#include "../../../../io/servo.h"

#include "../../../../modes/aircraft.h"

#include "test_pwm.h"

int api_test_pwm(const char *cmd, const char *args) {
    if (aircraft.mode == MODE_DIRECT) {
        uint in[] = {flash.pins[PINS_INPUT_AIL], flash.pins[PINS_INPUT_ELEV], flash.pins[PINS_INPUT_RUD],
                     flash.pins[PINS_INPUT_THROTTLE], flash.pins[PINS_INPUT_SWITCH]};
        uint out[] = {flash.pins[PINS_SERVO_AIL], flash.pins[PINS_SERVO_ELEV], flash.pins[PINS_SERVO_RUD],
                      flash.pins[PINS_ESC_THROTTLE], flash.pins[PINS_SERVO_DROP]};
        uint numBridges = 5;
        // Use bridges specified by command if any, if not the defaults will be kept
        if (args) {
            int numArgs = sscanf(args, "%d %d %d %d %d %d %d %d %d %d",
                                 &in[0], &out[0], &in[1], &out[1], &in[2], &out[2], &in[3], &out[3], &in[4], &out[4]);
            if (numArgs < 2 || numArgs % 2 != 0) return 400;
            numBridges = numArgs / 2;
        }
        // For every bridge, generate a random degree value and write it to the out pin, and read it back and compare
        for (uint i = 0; i < numBridges; i++) {
            printf("[api] testing pin combo %d:%d\n", in[i], out[i]);
            uint16_t deg = get_rand_32() % 180;
            servo_set(out[i], deg);
            sleep_ms(100);
            float degRead = pwm_read(in[i], PWM_MODE_DEG);
            if (fabsf(deg - degRead) > flash.control[CONTROL_DEADBAND]) {
                printf("[api] failed! read %f, expected %d\n", degRead, deg);
                return 500;
            }
        }
    } else return 403;
    return 200;
}
