#pragma once

#include <stdbool.h>
#include "platform/types.h"

typedef struct FusionConfig {
    f32 alpha;          // Complementary filter coefficient
    f32 sampleRate;     // Update rate (hz)
    f32 gyroBias[3];    // Gyroscope bias offsets [roll, pitch, yaw] (deg/s)
    f32 accelOffset[3]; // Accelerometer offsets [x, y, z] (g)
    bool useGPS;        // Whether to use GPS for yaw correction
} FusionConfig;

typedef struct FusionState {
    f32 roll;
    f32 pitch;
    f32 yaw;
    f32 rollRate;
    f32 pitchRate;
    f32 yawRate;
    u32 updateCount;
    bool initialized;
} FusionState;

typedef struct FusionCalibration {
    f32 gyroBiasSum[3];    // Sum of gyro readings for bias calculation
    f32 accelOffsetSum[3]; // Sum of accel readings for offset calculation
    u32 sampleCount;       // Number of samples collected
    bool isCalibrating;
} FusionCalibration;

/**
 * Initializes the sensor fusion system with default configuration.
 * @param config pointer to configuration structure to initialize
 * @param sampleRate update rate in hz
 */
void fusion_init(FusionConfig *config, f32 sampleRate);

/**
 * Resets the fusion state to initial values.
 * @param state pointer to state structure to reset
 */
void fusion_reset(FusionState *state);

/**
 * Updates the fusion filter with new sensor data.
 * @param config pointer to configuration
 * @param state pointer to state (will be updated)
 * @param accel accelerometer data [x, y, z] in g
 * @param gyro gyroscope data [x, y, z] in deg/s
 * @param gpsTrack GPS track (heading) in deg (optional, use NAN if not available)
 * @param dt time delta since last update in seconds
 */
void fusion_update(FusionConfig *config, FusionState *state, const f32 accel[3], const f32 gyro[3], f32 gpsTrack,
                   f32 dt);

/**
 * Begins calibration process.
 * @param cal pointer to calibration structure
 */
void fusion_calibration_start(FusionCalibration *cal);

/**
 * Adds a sample to the calibration process.
 * The device should be stationary and level during calibration.
 * @param cal pointer to calibration structure
 * @param accel accelerometer data [x, y, z] in g
 * @param gyro gyroscope data [x, y, z] in deg/s
 */
void fusion_calibration_add_sample(FusionCalibration *cal, const f32 accel[3], const f32 gyro[3]);

/**
 * Completes calibration and applies results to the configuration.
 * @param cal pointer to calibration structure
 * @param config pointer to configuration to update
 * @return true if calibration was successful
 */
bool fusion_calibration_finish(FusionCalibration *cal, FusionConfig *config);

/**
 * Loads calibration data from saved values.
 * @param config pointer to configuration to update
 * @param gyroBias gyroscope bias values [x, y, z] in deg/s
 * @param accelOffset accelerometer offset values [x, y, z] in g
 */
void fusion_load_calibration(FusionConfig *config, const f32 gyroBias[3], const f32 accelOffset[3]);
