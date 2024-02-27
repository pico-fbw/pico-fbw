/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <stdlib.h>

#include "io/servo.h"

#include "modes/aircraft.h"

#include "test_servo.h"

i32 api_test_servo(const char *cmd, const char *args) {
    if (aircraft.mode == MODE_DIRECT) {
        u32 num_servos = 3;
        u32 servos[num_servos];
        const u16 degrees[] = DEFAULT_SERVO_TEST;
        if (args) {
            // Test the servo that was provided in the command
            servos[0] = atoi(args);
            num_servos = 1;
            servo_test(servos, num_servos, degrees, NUM_DEFAULT_SERVO_TEST, DEFAULT_SERVO_TEST_PAUSE_MS);
        } else {
            // No arguments given, test with config values
            servo_getPins(servos, &num_servos);
            servo_test(servos, num_servos, degrees, NUM_DEFAULT_SERVO_TEST, DEFAULT_SERVO_TEST_PAUSE_MS);
        }
    } else return 403;
    return 200;
}
