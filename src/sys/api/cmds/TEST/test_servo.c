/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include <stdlib.h>
#include "pico/types.h"

#include "../../../../io/servo.h"

#include "../../../../modes/modes.h"

#include "test_servo.h"

uint api_test_servo(const char *cmd, const char *args) {
    if (getCurrentMode() == MODE_DIRECT) {
        uint num_servos = 3;
        uint servos[num_servos];
        const uint16_t degrees[] = DEFAULT_SERVO_TEST;
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
    } else {
        return 403;
    }
    return 200;
}
