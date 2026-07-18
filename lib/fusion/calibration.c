/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <math.h>
#include <string.h>

#include "sys/configuration.h"
#include "sys/print.h"

#include "calibration.h"

#define FUSION_CALIBRATION_SAMPLES 300           // Still samples required for baseline
#define FUSION_CALIBRATION_SAMPLE_INTERVAL_MS 10 // Min interval between calibration samples
#define FUSION_CALIBRATION_STILL_SETTLE_MS 1000  // Must remain still before counting samples
#define FUSION_CALIBRATION_MOTION_SETTLE_MS 450  // End motion step after this still window

// Pointers into runtime fusion state and remap outputs
static FusionConfig *activeFusionConfig = NULL;
static i8 *activeAxisMap = NULL;
static i8 *activeAxisSign = NULL;

// High-level status and working state for the calibration state machine
static IMUCalibrationStatus calibrationStatus = IMU_CALIBRATION_NOT_STARTED;
static FusionCalibrationContext calibrationCtx;
static FusionCalibrationDetails calibrationDetails;

// Validates a full 3-axis remap/sign set for uniqueness and bounds.
static bool axis_remap_valid(const i8 map[3], const i8 sign[3]) {
    bool used[3] = {false, false, false};
    for (u32 i = 0; i < 3; i++) {
        // Are axes within bounds?
        if (map[i] < 0 || map[i] > 2) {
            return false;
        }
        // Has this axis already been mapped?
        if (used[map[i]]) {
            return false;
        }
        used[map[i]] = true; // Now it has!
        // Are signs valid?
        if (sign[i] != 1 && sign[i] != -1) {
            return false;
        }
    }
    return true;
}

// Maps current motion step index to external status enum.
static IMUCalibrationStatus status_from_motion_axis(u32 motionAxis) {
    switch (motionAxis) {
        case 0:
            return IMU_CALIBRATION_ROLL_MAPPING;
        case 1:
            return IMU_CALIBRATION_PITCH_MAPPING;
        default:
            return IMU_CALIBRATION_YAW_MAPPING;
    }
}

// Human-readable user instruction for the requested motion step.
static const char *instruction_for_motion_axis(u32 motionAxis) {
    switch (motionAxis) {
        case 0:
            return "roll aircraft right, then return to level";
        case 1:
            return "pitch aircraft nose up, then return to level";
        default:
            return "yaw aircraft right, then return to level";
    }
}

// Converts raw sensor axis index to printable axis name.
static const char *sensor_axis_name(u32 sensorAxis) {
    switch (sensorAxis) {
        case 0:
            return "X";
        case 1:
            return "Y";
        default:
            return "Z";
    }
}

// Converts body axis index to printable axis name
static const char *body_axis_name(u32 bodyAxis) {
    switch (bodyAxis) {
        case 0:
            return "roll";
        case 1:
            return "pitch";
        default:
            return "yaw";
    }
}

// Returns default instruction text for a high-level calibration status.
static const char *instruction_for_status(IMUCalibrationStatus status) {
    switch (status) {
        case IMU_CALIBRATION_STATIONARY:
            return "Keep device level and stationary.";
        case IMU_CALIBRATION_ROLL_MAPPING:
            return "Roll aircraft right, then return to level.";
        case IMU_CALIBRATION_PITCH_MAPPING:
            return "Pitch aircraft nose up, then return to level.";
        case IMU_CALIBRATION_YAW_MAPPING:
            return "Yaw aircraft right, then return to level.";
        case IMU_CALIBRATION_APPLYING:
            return "Applying calibration...";
        case IMU_CALIBRATION_DONE:
            return "Calibration complete.";
        case IMU_CALIBRATION_FAILED:
            return "Calibration failed.";
        default:
            return "Calibration not started.";
    }
}

// Updates status and keeps the exported details snapshot in sync.
static void set_status(IMUCalibrationStatus status) {
    calibrationStatus = status;
    calibrationDetails.status = status;
    calibrationDetails.instructions = instruction_for_status(status);
}

// Updates only the supplementary status note.
static void set_note(const char *note) {
    calibrationDetails.note = note;
}

// Updates status and supplementary note together.
static void set_status_with_note(IMUCalibrationStatus status, const char *note) {
    set_status(status);
    set_note(note);
}

// Initializes all details fields to safe defaults.
static void reset_details() {
    memset(&calibrationDetails, 0, sizeof(calibrationDetails));
    calibrationDetails.status = IMU_CALIBRATION_NOT_STARTED;
    calibrationDetails.instructions = instruction_for_status(IMU_CALIBRATION_NOT_STARTED);
    calibrationDetails.stillSampleTarget = FUSION_CALIBRATION_SAMPLES;
    calibrationDetails.lastMappedBodyAxis = -1;
    calibrationDetails.lastMappedSensorAxis = -1;
    calibrationDetails.lastMappedSensorSign = 0;
}

// Refreshes rolling counters/flags from calibration context into details snapshot.
static void refresh_details_runtime() {
    calibrationDetails.stillSampleCount = calibrationCtx.stillSampleCount;
    calibrationDetails.motionAxisIndex = calibrationCtx.motionAxisIndex;
    calibrationDetails.motionDetected = calibrationCtx.motionDetected;
}

// Initializes the state machine for a new fusion calibration run.
static void begin_calibration() {
    memset(&calibrationCtx, 0, sizeof(calibrationCtx));
    calibrationCtx.stillSince = timestamp_now();
    calibrationCtx.lastSample = timestamp_now();
    for (u32 i = 0; i < 3; i++) {
        calibrationCtx.axisMap[i] = -1;
        calibrationCtx.axisSign[i] = 1;
    }

    reset_details();
    set_status(IMU_CALIBRATION_STATIONARY);
    printsys(imu, "starting fusion calibration");
    printsys(imu, "keep aircraft level and steady while baseline is captured");
}

// Finalizes and persists offsets/remap/sign parameters after successful capture.
static bool finish_calibration() {
    if (!activeFusionConfig || !activeAxisMap || !activeAxisSign) {
        return false;
    }
    // A complete 3-axis map and at least one still sample are required
    if (!axis_remap_valid(calibrationCtx.axisMap, calibrationCtx.axisSign) || calibrationCtx.stillSampleCount == 0) {
        return false;
    }

    f32 mappedStillAccel[3] = {0.f, 0.f, 0.f};
    f32 mappedStillGyro[3] = {0.f, 0.f, 0.f};
    for (u32 i = 0; i < 3; i++) {
        i8 sensorAxis = calibrationCtx.axisMap[i];
        // Offsets are stored in permuted sensor frame
        // Axis sign is applied to exported RPY/rates
        mappedStillAccel[i] = calibrationCtx.stillAccelAvg[sensorAxis];
        mappedStillGyro[i] = calibrationCtx.stillGyroAvg[sensorAxis];
    }

    f32 accelOffset[3] = {
        mappedStillAccel[0],
        mappedStillAccel[1],
        // In this fusion convention, level should resolve to +1g on mapped Z
        mappedStillAccel[2] - 1.0f,
    };
    // Push freshly computed still offsets directly into active fusion config
    fusion_load_calibration(activeFusionConfig, mappedStillGyro, accelOffset);

    // Publish the solved remap/sign to runtime fusion consumers.
    memcpy(activeAxisMap, calibrationCtx.axisMap, 3 * sizeof(i8));
    memcpy(activeAxisSign, calibrationCtx.axisSign, 3 * sizeof(i8));

    // Persist calibration so it is restored on next boot
    calibration.imu.calibrated = true;
    calibration.imu.gyroBiasX = activeFusionConfig->gyroBias[0];
    calibration.imu.gyroBiasY = activeFusionConfig->gyroBias[1];
    calibration.imu.gyroBiasZ = activeFusionConfig->gyroBias[2];
    calibration.imu.accelOffsetX = activeFusionConfig->accelOffset[0];
    calibration.imu.accelOffsetY = activeFusionConfig->accelOffset[1];
    calibration.imu.accelOffsetZ = activeFusionConfig->accelOffset[2];
    calibration.imu.axisMapRoll = (f32)activeAxisMap[0];
    calibration.imu.axisMapPitch = (f32)activeAxisMap[1];
    calibration.imu.axisMapYaw = (f32)activeAxisMap[2];
    calibration.imu.axisSignRoll = (f32)activeAxisSign[0];
    calibration.imu.axisSignPitch = (f32)activeAxisSign[1];
    calibration.imu.axisSignYaw = (f32)activeAxisSign[2];
    config_save();

    imu.isCalibrated = true;
    set_status_with_note(IMU_CALIBRATION_DONE, "Calibration saved.");
    printsys(imu, "calibration successful");
    printsys(imu, "gyro bias: [%.4f, %.4f, %.4f] deg/s", activeFusionConfig->gyroBias[0],
             activeFusionConfig->gyroBias[1], activeFusionConfig->gyroBias[2]);
    printsys(imu, "accel offset: [%.4f, %.4f, %.4f] g", activeFusionConfig->accelOffset[0],
             activeFusionConfig->accelOffset[1], activeFusionConfig->accelOffset[2]);
    return true;
}

void fusion_attitude_calibration_reset_axis_remap(i8 axisMap[3], i8 axisSign[3]) {
    axisMap[0] = 0;
    axisMap[1] = 1;
    axisMap[2] = 2;
    axisSign[0] = 1;
    axisSign[1] = 1;
    axisSign[2] = 1;
}

bool fusion_attitude_calibration_load_axis_remap(i8 axisMap[3], i8 axisSign[3]) {
    i8 loadedMap[3] = {
        (i8)calibration.imu.axisMapRoll,
        (i8)calibration.imu.axisSignPitch,
        (i8)calibration.imu.axisMapYaw,
    };
    i8 loadedSign[3] = {
        (i8)calibration.imu.axisSignRoll,
        (i8)calibration.imu.axisSignPitch,
        (i8)calibration.imu.axisSignYaw,
    };
    if (!axis_remap_valid(loadedMap, loadedSign)) {
        return false;
    }
    memcpy(axisMap, loadedMap, sizeof(loadedMap));
    memcpy(axisSign, loadedSign, sizeof(loadedSign));
    return true;
}

void fusion_attitude_calibration_init(FusionConfig *fusionConfig, i8 axisMap[3], i8 axisSign[3]) {
    // Bind runtime storage used by the calibration flow
    activeFusionConfig = fusionConfig;
    activeAxisMap = axisMap;
    activeAxisSign = axisSign;
    set_status(IMU_CALIBRATION_NOT_STARTED);
    memset(&calibrationCtx, 0, sizeof(calibrationCtx));
    reset_details();
}

void fusion_attitude_calibration_deinit() {
    // Clear external bindings and reset state-machine internals
    activeFusionConfig = NULL;
    activeAxisMap = NULL;
    activeAxisSign = NULL;
    set_status(IMU_CALIBRATION_NOT_STARTED);
    memset(&calibrationCtx, 0, sizeof(calibrationCtx));
    reset_details();
}

IMUCalibrationStatus fusion_attitude_calibration_start() {
    // Start is idempotent: once running, this just returns current state
    if (!activeFusionConfig || !activeAxisMap || !activeAxisSign) {
        set_status_with_note(IMU_CALIBRATION_FAILED, "Calibration subsystem not initialized.");
        return calibrationStatus;
    }
    if (calibrationStatus == IMU_CALIBRATION_NOT_STARTED) {
        begin_calibration();
    }
    return calibrationStatus;
}

IMUCalibrationStatus fusion_attitude_calibration_status() {
    return calibrationStatus;
}

void fusion_attitude_calibration_get_details(FusionCalibrationDetails *details) {
    if (!details) {
        return;
    }
    refresh_details_runtime();
    *details = calibrationDetails;
}

void fusion_attitude_calibration_update(const f32 accelAvg[3], const f32 gyroAvg[3]) {
    // Only run active calibration phases; done/failed/not-started states are inert
    if (!activeFusionConfig || !activeAxisMap || !activeAxisSign) {
        return;
    }
    if (calibrationStatus == IMU_CALIBRATION_NOT_STARTED || calibrationStatus == IMU_CALIBRATION_DONE ||
        calibrationStatus == IMU_CALIBRATION_FAILED) {
        return;
    }

    if (time_since_ms(&calibrationCtx.lastSample) < FUSION_CALIBRATION_SAMPLE_INTERVAL_MS) {
        return;
    }
    // Throttle calibration processing to a fixed sample cadence
    calibrationCtx.lastSample = timestamp_now();
    refresh_details_runtime();

    // Step 1: collect stable baseline samples while level and still
    if (calibrationStatus == IMU_CALIBRATION_STATIONARY) {
        bool still = fusion_attitude_calibration_level_and_still(accelAvg, gyroAvg);
        if (!still) {
            if (!calibrationCtx.warnedMoving) {
                printsys(imu, "movement detected; waiting for aircraft to become still and level");
                calibrationCtx.warnedMoving = true;
            }
            // Break the still window so the settle timer restarts once motion stops
            calibrationCtx.stillWindowActive = false;
            calibrationDetails.waitingForStillness = true;
            set_note("Movement detected; waiting for stillness.");
            return;
        }

        calibrationCtx.warnedMoving = false;
        calibrationDetails.waitingForStillness = false;
        set_note(NULL);
        if (!calibrationCtx.stillWindowActive) {
            // First still sample after motion: start settle timer before counting samples
            calibrationCtx.stillWindowActive = true;
            calibrationCtx.stillSince = timestamp_now();
            if (calibrationCtx.stillSampleCount == 0) {
                printsys(imu, "stillness detected, gathering baseline samples");
                set_note("Stillness detected, collecting baseline.");
            }
            return;
        }
        if (time_since_ms(&calibrationCtx.stillSince) < FUSION_CALIBRATION_STILL_SETTLE_MS) {
            // Require continuous stillness before accepting baseline samples
            return;
        }

        fusion_attitude_calibration_still_add_sample(&calibrationCtx, accelAvg, gyroAvg);
        if (calibrationCtx.stillSampleCount % 50 == 0) {
            printsys(imu, "still calibration progress: %u/%d", (unsigned int)calibrationCtx.stillSampleCount,
                     FUSION_CALIBRATION_SAMPLES);
        }

        if (calibrationCtx.stillSampleCount >= FUSION_CALIBRATION_SAMPLES) {
            // Baseline complete; switch to first motion mapping step (roll).
            fusion_attitude_calibration_still_finalize(&calibrationCtx);
            calibrationCtx.motionAxisIndex = 0;
            set_status(IMU_CALIBRATION_ROLL_MAPPING);
            fusion_attitude_calibration_motion_reset(&calibrationCtx);
            calibrationDetails.axisMappingUnclear = false;
            set_note("Baseline complete.");
            printsys(imu, "baseline complete; %s", instruction_for_motion_axis(calibrationCtx.motionAxisIndex));
        }
        return;
    }

    // Step 2: capture one deliberate motion per body axis to resolve axis order/sign.
    if (calibrationStatus == IMU_CALIBRATION_ROLL_MAPPING || calibrationStatus == IMU_CALIBRATION_PITCH_MAPPING ||
        calibrationStatus == IMU_CALIBRATION_YAW_MAPPING) {
        f32 centeredGyro[3] = {
            gyroAvg[0] - calibrationCtx.stillGyroAvg[0],
            gyroAvg[1] - calibrationCtx.stillGyroAvg[1],
            gyroAvg[2] - calibrationCtx.stillGyroAvg[2],
        };

        if (fusion_attitude_calibration_motion_active(centeredGyro)) {
            if (!calibrationCtx.motionDetected) {
                // Start a fresh capture window the first time motion crosses threshold.
                fusion_attitude_calibration_motion_reset(&calibrationCtx);
                calibrationCtx.motionDetected = true;
                set_note("Motion detected; capturing axis response.");
            }
            // Continue integrating motion evidence for dominant axis/sign inference.
            fusion_attitude_calibration_motion_accumulate(&calibrationCtx, centeredGyro);
            calibrationCtx.lastMotion = timestamp_now();
            return;
        }

        if (!calibrationCtx.motionDetected ||
            time_since_ms(&calibrationCtx.lastMotion) < FUSION_CALIBRATION_MOTION_SETTLE_MS) {
            // Wait for motion to end and settle before scoring this maneuver.
            return;
        }

        i8 sensorAxis = 0;
        i8 sensorSign = 1;
        if (!fusion_attitude_calibration_motion_select_axis(&calibrationCtx, &sensorAxis, &sensorSign)) {
            printsys(imu, "axis mapping was unclear; retry: %s",
                     instruction_for_motion_axis(calibrationCtx.motionAxisIndex));
            // Capture quality was ambiguous; reset and retry this same step.
            fusion_attitude_calibration_motion_reset(&calibrationCtx);
            calibrationDetails.axisMappingUnclear = true;
            set_note("Axis mapping was unclear; retrying step.");
            return;
        }

        calibrationCtx.axisMap[calibrationCtx.motionAxisIndex] = sensorAxis;
        calibrationCtx.axisSign[calibrationCtx.motionAxisIndex] = sensorSign;
        calibrationCtx.rawAxisUsed[sensorAxis] = true;
        calibrationDetails.axisMappingUnclear = false;
        calibrationDetails.lastMappedBodyAxis = (i8)calibrationCtx.motionAxisIndex;
        calibrationDetails.lastMappedSensorAxis = sensorAxis;
        calibrationDetails.lastMappedSensorSign = sensorSign;
        set_note("Axis mapping accepted.");
        printsys(imu, "mapped %s to sensor %s axis (%s)", body_axis_name(calibrationCtx.motionAxisIndex),
                 sensor_axis_name((u32)sensorAxis), sensorSign > 0 ? "positive" : "negative");

        calibrationCtx.motionAxisIndex++;
        fusion_attitude_calibration_motion_reset(&calibrationCtx);
        if (calibrationCtx.motionAxisIndex >= 3) {
            // Roll/pitch/yaw mapping complete; move to apply/persist phase
            set_status_with_note(IMU_CALIBRATION_APPLYING, "All axis mappings captured.");
            return;
        }

        // Continue to next required body-axis maneuver
        set_status(status_from_motion_axis(calibrationCtx.motionAxisIndex));
        printsys(imu, "%s", instruction_for_motion_axis(calibrationCtx.motionAxisIndex));
        return;
    }

    if (calibrationStatus == IMU_CALIBRATION_APPLYING) {
        // Finalize, write, and publish all solved calibration parameters
        if (!finish_calibration()) {
            set_status_with_note(IMU_CALIBRATION_FAILED, "Failed to apply captured calibration values.");
        }
        return;
    }

    set_status_with_note(IMU_CALIBRATION_FAILED, "Unexpected calibration state.");
}
