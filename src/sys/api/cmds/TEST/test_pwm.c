/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include "platform/helpers.h"
#include "platform/time.h"

#include "ctrl/aircraft.h"
#include "io/receiver.h"
#include "io/servo.h"
#include "lib/parson.h"
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
static u32 parse_args(const char *args, i16 in[], i16 out[]) {
    JSON_Value *root = json_parse_string(args);
    if (!root) {
        return 0;
    }
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
        in[i] = (i16)json_array_get_number(bridge, 0);
        out[i] = (i16)json_array_get_number(bridge, 1);
    }
    json_value_free(root);
    return bridges;
}

// {"bridges":[[number,number],...]}

i32 api_test_pwm(const char *args) {
    if (aircraft_get_mode() != MODE_DIRECT) {
        return 403;
    }

    // These pins should be bridged by the user
    i16 in[] = {(i16)config.pins.inputAil, (i16)config.pins.inputEle, (i16)config.pins.inputRud,
                (i16)config.pins.inputThrottle, (i16)config.pins.inputSwitch};
    i16 out[] = {(i16)config.pins.servoAil, (i16)config.pins.servoEle, (i16)config.pins.servoRud,
                 (i16)config.pins.escThrottle, (i16)config.pins.servoBay};
    u32 numBridges = 5;
    if (args) {
        // If the user specified any bridges, use those instead
        numBridges = parse_args(args, in, out);
        if (numBridges == 0) {
            return 400;
        }
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
        printpre("test", "testing pin combo %d:%d", in[i], out[i]);
        f32 deg = testDegrees[i % (count_of(testDegrees))];
        servo_set(out[i], deg);
        sleep_ms_blocking(100);
        f32 degRead = receiver_get(in[i], RECEIVER_MODE_DEGREE);
        if (fabsf(deg - degRead) > config.control.controlDeadband) {
            printpre("test", "failed! read %.0f, expected %.0f", degRead, deg);
            return 500;
        }
    }
    return 200;
}
