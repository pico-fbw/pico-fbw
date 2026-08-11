/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <string.h>

#include "ctrl/aircraft.h"
#include "lib/fusion/calibration.h"
#include "lib/parson.h"

#include "io/esc.h"
#include "io/imu.h"
#include "io/receiver.h"

#include "sys/configuration.h"

#include "set.h"

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

/**
 * Serializes detailed fusion calibration status to API output.
 * @param details detailed calibration snapshot
 * @param out output JSON string (allocated on success)
 * @return true on success
 */
static bool serialize_fusion_attitude_calibration_status(const FusionCalibrationDetails *details, char **out) {
    JSON_Value *root = json_value_init_object();
    if (!root) {
        return false;
    }
    JSON_Object *obj = json_value_get_object(root);
    if (!obj) {
        json_value_free(root);
        return false;
    }
    // All important values from calibration are serialized
    json_object_set_number(obj, "status", (f64)details->status);
    json_object_set_string(obj, "instructions", details->instructions ? details->instructions : "");
    if (details->note) {
        json_object_set_string(obj, "note", details->note);
    } else {
        json_object_set_null(obj, "note");
    }
    json_object_set_number(obj, "still_samples", (f64)details->stillSampleCount);
    json_object_set_number(obj, "still_sample_target", (f64)details->stillSampleTarget);
    json_object_set_number(obj, "motion_axis_index", (f64)details->motionAxisIndex);
    json_object_set_boolean(obj, "waiting_for_stillness", details->waitingForStillness);
    json_object_set_boolean(obj, "motion_detected", details->motionDetected);
    json_object_set_boolean(obj, "axis_mapping_unclear", details->axisMappingUnclear);
    json_object_set_number(obj, "last_mapped_body_axis", (f64)details->lastMappedBodyAxis);
    json_object_set_number(obj, "last_mapped_sensor_axis", (f64)details->lastMappedSensorAxis);
    json_object_set_number(obj, "last_mapped_sensor_sign", (f64)details->lastMappedSensorSign);

    char *serialized = json_serialize_to_string(root);
    json_value_free(root);
    if (!serialized) {
        return false;
    }
    *out = serialized;
    return true;
}

// Input:
// {"system":"receiver|esc|imu"}

// Output:
// {"status":number,"instructions":"","note":string|null,"still_samples":number,"still_sample_target":number,
//  "motion_axis_index":number,"waiting_for_stillness":bool,"motion_detected":bool,"axis_mapping_unclear":bool,
//  "last_mapped_body_axis":number,"last_mapped_sensor_axis":number,"last_mapped_sensor_sign":number}

i32 api_set_calibration(const char *in, char **out) {
    if (aircraft_is_flying()) {
        return 403;
    }
    CalibrationSystem system = parse_args(in);
    bool completed = false;
    switch (system) {
        // TODO: add output messages for receiver and esc calibration as well
        case CALIBRATION_RECEIVER: {
            u32 numPins = MAX_RECEIVER_PINS;
            i16 pins[numPins];
            f32 deviations[numPins];
            receiver_get_pins(pins, &numPins, deviations);
            completed = receiver_calibrate(pins, numPins, deviations, DEFAULT_RECEIVER_CALIBRATION_SAMPLES,
                                           DEFAULT_RECEIVER_CALIBRATION_SAMPLE_DELAY_MS,
                                           DEFAULT_RECEIVER_CALIBRATION_RUN_TIMES) &&
                        receiver_is_calibrated() == RECEIVERCALIBRATION_OK;
            break;
        }
        case CALIBRATION_ESCS:
            completed = esc_calibrate((i16)config.pins.escThrottle);
            break;
        case CALIBRATION_IMU: {
            IMUCalibrationStatus status = imu.calibrate();
            FusionCalibrationDetails details = {};
            fusion_attitude_calibration_get_details(&details);
            if (!serialize_fusion_attitude_calibration_status(&details, out)) {
                return 500;
            }
            switch (status) {
                case IMU_CALIBRATION_DONE:
                    completed = true;
                    return 200;
                case IMU_CALIBRATION_FAILED:
                    return 500;
                default:
                    return 202;
            }
            break;
        }
        default:
            return 400;
    }
    return completed ? 200 : 500;
}
