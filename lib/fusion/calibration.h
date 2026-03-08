#pragma once

#include "platform/time.h"
#include "platform/types.h"

#include "io/imu.h"
#include "lib/fusion/fusion.h"

// Context struct to be passed around the calibration state machine
typedef struct FusionCalibrationContext {
    // Accumulated still samples used for baseline bias/offset estimation
    f32 stillAccelSum[3];
    f32 stillGyroSum[3];
    // Finalized still-sample averages once baseline capture completes
    f32 stillAccelAvg[3];
    f32 stillGyroAvg[3];

    // Motion-capture statistics used to infer dominant axis/sign per maneuver
    f32 motionPeakAbs[3];
    f32 motionAbsSum[3];
    f32 motionSignedSum[3];
    i8 motionInitialSign[3];

    // Axis-remap solution under construction across roll/pitch/yaw steps
    bool rawAxisUsed[3];
    i8 axisMap[3];
    i8 axisSign[3];

    // Progress counters/state for still and motion capture phases
    u32 stillSampleCount;
    u32 motionAxisIndex;
    bool stillWindowActive;
    bool motionDetected;
    bool warnedMoving;

    // Timing gates used to debounce stillness and motion windows
    Timestamp stillSince;
    Timestamp lastSample;
    Timestamp lastMotion;
} FusionCalibrationContext;

typedef struct FusionCalibrationDetails {
    IMUCalibrationStatus status; // Current high-level calibration step
    const char *instructions;    // Current user instruction for the active step
    const char *note;            // Supplemental status detail (may be NULL)
    u32 stillSampleCount;        // Number of still baseline samples collected
    u32 stillSampleTarget;       // Number of still samples required
    u32 motionAxisIndex;         // Current body-axis step: 0=roll, 1=pitch, 2=yaw
    bool waitingForStillness;    // True while baseline step is blocked by motion
    bool motionDetected;         // True while a motion-capture window is active
    bool axisMappingUnclear;     // True when the previous mapping attempt was rejected and retried
    i8 lastMappedBodyAxis;       // Last mapped body axis, or -1 if none
    i8 lastMappedSensorAxis;     // Last mapped raw sensor axis, or -1 if none
    i8 lastMappedSensorSign;     // Last mapped sign (+1/-1), or 0 if none
} FusionCalibrationDetails;

/**
 * Resets remap arrays to identity defaults.
 * @param axisMap output remap array [roll, pitch, yaw] -> [sensor axis index]
 * @param axisSign output sign array [roll, pitch, yaw] -> {-1, +1}
 */
void fusion_attitude_calibration_reset_axis_remap(i8 axisMap[3], i8 axisSign[3]);

/**
 * Loads axis remap/sign configuration from persisted calibration storage.
 * @param axisMap output remap array [roll, pitch, yaw] -> [sensor axis index]
 * @param axisSign output sign array [roll, pitch, yaw] -> {-1, +1}
 * @return true if the stored remap/sign set is valid and loaded
 */
bool fusion_attitude_calibration_load_axis_remap(i8 axisMap[3], i8 axisSign[3]);

/**
 * Initializes the fusion calibration module and binds live fusion/remap targets.
 * @param fusionConfig fusion configuration that calibration writes biases/offsets into
 * @param axisMap runtime axis map used by fusion consumers
 * @param axisSign runtime axis signs used by fusion output mapping
 */
void fusion_attitude_calibration_init(FusionConfig *fusionConfig, i8 axisMap[3], i8 axisSign[3]);

/**
 * Deinitializes the fusion calibration module and clears internal state.
 */
void fusion_attitude_calibration_deinit();

/**
 * Starts the fusion calibration sequence if not already started.
 * @return current calibration status after attempting to start
 */
IMUCalibrationStatus fusion_attitude_calibration_start();

/**
 * Returns the current high-level fusion calibration status.
 * @return current calibration status
 */
IMUCalibrationStatus fusion_attitude_calibration_status();

/**
 * Returns a detailed snapshot of the fusion calibration process.
 * @param details output structure to populate
 */
void fusion_attitude_calibration_get_details(FusionCalibrationDetails *details);

/**
 * Advances the fusion calibration state machine using latest sensor averages.
 * This function is designed to be called from the high-rate fusion update loop.
 * @param accelAvg current averaged accelerometer sample [x, y, z] in g
 * @param gyroAvg current averaged gyroscope sample [x, y, z] in deg/s
 */
void fusion_attitude_calibration_update(const f32 accelAvg[3], const f32 gyroAvg[3]);

/* -- Still -- */

/**
 * Evaluates level/still constraints for baseline capture
 * (low gyro rate, near-1g magnitude, and one dominant gravity axis).
 */
bool fusion_attitude_calibration_level_and_still(const f32 accel[3], const f32 gyro[3]);

/** Accumulates one still baseline sample into the still-calibration sums. */
void fusion_attitude_calibration_still_add_sample(FusionCalibrationContext *ctx, const f32 accel[3], const f32 gyro[3]);

/** Finalizes still baseline averages from accumulated still samples. */
void fusion_attitude_calibration_still_finalize(FusionCalibrationContext *ctx);

/* -- Motion -- */

/** Clears motion capture accumulators and starts a fresh motion window. */
void fusion_attitude_calibration_motion_reset(FusionCalibrationContext *ctx);

/** Returns true when a centered-gyro sample indicates active axis motion. */
bool fusion_attitude_calibration_motion_active(const f32 centeredGyro[3]);

/** Accumulates one centered-gyro sample into current motion capture statistics. */
void fusion_attitude_calibration_motion_accumulate(FusionCalibrationContext *ctx, const f32 centeredGyro[3]);

/**
 * Selects the best raw axis/sign mapping from captured motion data.
 * @param ctx captured motion context
 * @param sensorAxis output dominant raw sensor axis index
 * @param sensorSign output dominant raw axis sign (+1 or -1)
 * @return true if the capture quality was sufficient to accept a mapping
 */
bool fusion_attitude_calibration_motion_select_axis(const FusionCalibrationContext *ctx, i8 *sensorAxis,
                                                    i8 *sensorSign);
