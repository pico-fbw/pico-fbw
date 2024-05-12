/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/helpers.h"

#include "io/servo.h"

#include "lib/parson.h"

#include "modes/aircraft.h"

#include "test_servo.h"

/**
 * Helper to parse command arguments.
 * @param args command arguments
 * @param servos array to store servo pins
 * @return number of servos parsed, or 0 if parsing failed
 */
static u32 parse_args(const char *args, u32 servos[]) {
    JSON_Value *root = json_parse_string(args);
    if (!root)
        return 0;
    JSON_Object *obj = json_value_get_object(root);
    if (!obj) {
        json_value_free(root);
        return 0;
    }
    JSON_Array *arr = json_object_get_array(obj, "servos");
    if (!arr) {
        json_value_free(root);
        return 0;
    }
    u32 numServos = json_array_get_count(arr);
    if (numServos < 1 || numServos > 3) {
        json_value_free(root);
        return 0;
    }
    for (u32 i = 0; i < numServos; i++)
        servos[i] = (u32)json_array_get_number(arr, i);
    json_value_free(root);
    return numServos;
}

// {"servos":[number,...]}

i32 api_test_servo(const char *args) {
    if (aircraft.mode != MODE_DIRECT)
        return 403;

    u32 numServos = 3;
    u32 servos[numServos];
    const f32 degrees[] = DEFAULT_SERVO_TEST;
    if (args) {
        // Test the servo(s) provided in the command
        numServos = parse_args(args, servos);
        if (numServos == 0)
            return 400;
    } else
        // No arguments given, test with all servos (that are relavent in the current control mode)
        servo_get_pins(servos, &numServos);

    servo_test(servos, numServos, degrees, count_of(degrees), DEFAULT_SERVO_TEST_PAUSE_MS);
    return 200;
}
