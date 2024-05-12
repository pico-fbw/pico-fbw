/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include "platform/helpers.h"
#include "platform/time.h"

#include "io/receiver.h"
#include "io/servo.h"

#include "lib/parson.h"

#include "modes/aircraft.h"

#include "sys/configuration.h"
#include "sys/print.h"

#include "test_pwm.h"

/**
 * @return a pseudo-randomly generated number between 0 and 180
 */
static f32 rand_180() {
    srand((u32)time_us());
    return (f32)rand() / RAND_MAX * 180;
}

/**
 * Helper to parse command arguments.
 * @param args command arguments
 * @param in array to store input pins
 * @param out array to store output pins
 * @return number of bridges parsed, or 0 if parsing failed
 */
static u32 parse_args(const char *args, u32 in[], u32 out[]) {
    JSON_Value *root = json_parse_string(args);
    if (!root)
        return 0;
    JSON_Object *obj = json_value_get_object(root);
    if (!obj) {
        json_value_free(root);
        return 0;
    }
    JSON_Array *arr = json_object_get_array(obj, "bridges");
    if (!arr) {
        json_value_free(root);
        return 0;
    }
    u32 bridges = json_array_get_count(arr);
    if (bridges < 1 || bridges > 5) {
        json_value_free(root);
        return 0;
    }
    for (u32 i = 0; i < bridges; i++) {
        JSON_Array *bridge = json_array_get_array(arr, i);
        if (json_array_get_count(bridge) != 2) {
            json_value_free(root);
            return 0;
        }
        in[i] = (u32)json_array_get_number(bridge, 0);
        out[i] = (u32)json_array_get_number(bridge, 1);
    }
    json_value_free(root);
    return bridges;
}

// {"bridges":[[number,number],...]}

i32 api_test_pwm(const char *args) {
    if (aircraft.mode != MODE_DIRECT)
        return 403;

    // These pins should be bridged by the user
    u32 in[] = {config.pins[PINS_INPUT_AIL], config.pins[PINS_INPUT_ELE], config.pins[PINS_INPUT_RUD],
                config.pins[PINS_INPUT_THROTTLE], config.pins[PINS_INPUT_SWITCH]};
    u32 out[] = {config.pins[PINS_SERVO_AIL], config.pins[PINS_SERVO_ELE], config.pins[PINS_SERVO_RUD],
                 config.pins[PINS_ESC_THROTTLE], config.pins[PINS_SERVO_BAY]};
    u32 numBridges = 5;
    if (args) {
        // If the user specified any bridges, use those instead
        numBridges = parse_args(args, in, out);
        if (numBridges == 0)
            return 400;
    }
    // Generate some "random" degree values to test with
    f32 testDegrees[numBridges];
    for (u32 i = 0; i < count_of(testDegrees); i++) {
        f32 randNum = rand_180();
        testDegrees[i] = (int)(randNum * 100) / 100.0; // Round to 2 decimal places; PWM system is not insanely accurate
        sleep_ms_blocking((u64)randNum);               // Sleep a bit to get a new seed
    }
    // For every bridge, set the degree value from the predefined set and compare the read value
    for (u32 i = 0; i < numBridges; i++) {
        printpre("test", "testing pin combo %lu:%lu", in[i], out[i]);
        f32 deg = testDegrees[i % (count_of(testDegrees))];
        servo_set(out[i], deg);
        sleep_ms_blocking(100);
        f32 degRead = receiver_get(in[i], RECEIVER_MODE_DEGREE);
        if (fabsf(deg - degRead) > config.control[CONTROL_DEADBAND]) {
            printpre("test", "failed! read %.0f, expected %.0f", degRead, deg);
            return 500;
        }
    }
    return 200;
}
