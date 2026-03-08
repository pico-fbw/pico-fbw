/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <string.h>

#include "lib/parson.h"

#include "io/esc.h"
#include "io/imu.h"
#include "io/receiver.h"

#include "get_calibration.h"

typedef enum CalibrationSystem {
    CALIBRATION_RECEIVER,
    CALIBRATION_ESCS,
    CALIBRATION_IMU,
    CALIBRATION_UNKNOWN,
} CalibrationSystem;

/**
 * Helper to parse command arguments.
 * @param args command arguments
 * @return system to calibrate
 */
static CalibrationSystem parse_args(const char *args) {
    JSON_Value *root = json_parse_string(args);
    if (!root) {
        return CALIBRATION_UNKNOWN;
    }
    JSON_Object *obj = json_value_get_object(root);
    if (!obj) {
        json_value_free(root);
        return CALIBRATION_UNKNOWN;
    }
    const char *systemStr = json_object_get_string(obj, "system");
    if (!systemStr) {
        json_value_free(root);
        return CALIBRATION_UNKNOWN;
    }
    CalibrationSystem system;
    if (strcasecmp(systemStr, "receiver") == 0) {
        system = CALIBRATION_RECEIVER;
    } else if (strcasecmp(systemStr, "esc") == 0) {
        system = CALIBRATION_ESCS;
    } else if (strcasecmp(systemStr, "imu") == 0) {
        system = CALIBRATION_IMU;
    } else {
        system = CALIBRATION_UNKNOWN;
    }
    json_value_free(root);
    return system;
}

// Input:
// {"system":"receiver|esc|imu"}

// Output:
// {"calibrated":boolean}

i32 api_get_calibration(const char *in, char **out) {
    JSON_Value *root = json_value_init_object();
    JSON_Object *obj = json_value_get_object(root);
    CalibrationSystem system = parse_args(in);
    bool calibrated = false;
    switch (system) {
        case CALIBRATION_RECEIVER:
            calibrated = receiver_is_calibrated() == RECEIVERCALIBRATION_OK;
            break;
        case CALIBRATION_ESCS:
            calibrated = esc_is_calibrated();
            break;
        case CALIBRATION_IMU:
            calibrated = imu.isCalibrated;
            break;
        default:
            return 400;
    }
    json_object_set_boolean(obj, "calibrated", calibrated);
    char *serialized = json_serialize_to_string(root);
    json_value_free(root);
    *out = serialized;
    return 200;
}
