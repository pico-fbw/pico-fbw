/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <math.h>
#include <stdbool.h>

#include "lib/parson.h"

#include "modes/aircraft.h"
#include "modes/normal.h"

#include "sys/configuration.h"

#include "set_target.h"

/**
 * Helper to parse command arguments.
 * @param args command arguments
 * @param roll pointer to store the roll setpoint
 * @param pitch pointer to store the pitch setpoint
 * @param yaw pointer to store the yaw setpoint
 * @param throttle pointer to store the throttle setpoint
 * @return true if the arguments were parsed successfully
 */
static bool parse_args(const char *args, f32 *roll, f32 *pitch, f32 *yaw, f32 *throttle) {
    JSON_Value *root = json_parse_string(args);
    if (!root)
        return false;
    JSON_Object *obj = json_value_get_object(root);
    if (!obj) {
        json_value_free(root);
        return false;
    }
    JSON_Value *rollVal = json_object_get_value(obj, "roll");
    if (!rollVal || json_value_get_type(rollVal) != JSONNumber) {
        json_value_free(root);
        return false;
    }
    JSON_Value *pitchVal = json_object_get_value(obj, "pitch");
    if (!pitchVal || json_value_get_type(pitchVal) != JSONNumber) {
        json_value_free(root);
        return false;
    }
    JSON_Value *yawVal = json_object_get_value(obj, "yaw");
    if (!yawVal || json_value_get_type(yawVal) != JSONNumber) {
        json_value_free(root);
        return false;
    }
    JSON_Value *throttleVal = json_object_get_value(obj, "throttle");
    if (!throttleVal || json_value_get_type(throttleVal) != JSONNumber) {
        json_value_free(root);
        return false;
    }
    *roll = (f32)json_value_get_number(rollVal);
    *pitch = (f32)json_value_get_number(pitchVal);
    *yaw = (f32)json_value_get_number(yawVal);
    *throttle = (f32)json_value_get_number(throttleVal);
    json_value_free(root);
    return true;
}

// {"roll":number,"pitch":number,"yaw":number,"throttle":number}

i32 api_set_target(const char *args) {
    if (aircraft.mode != MODE_NORMAL)
        return 403;

    f32 roll, pitch, yaw, throttle;
    if (!parse_args(args, &roll, &pitch, &yaw, &throttle))
        return 400;
    // Ensure setpoints are within limits
    if (fabsf(roll) > config.control[CONTROL_ROLL_LIMIT_HOLD] || pitch > config.control[CONTROL_PITCH_UPPER_LIMIT] ||
        pitch < config.control[CONTROL_PITCH_LOWER_LIMIT] || fabsf(yaw) > config.control[CONTROL_MAX_RUD_DEFLECTION])
        return 400;
    if (throttle < 0.f || throttle > 100.f)
        return 400;
    // Pass the setpoints into normal mode, 423 will be returned if the mode rejects the code (user input takes priority)
    return normal_set(roll, pitch, yaw, throttle) ? 200 : 423;
}
