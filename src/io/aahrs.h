#pragma once

#include <stdbool.h>
#include "platform/types.h"

#define IMU_MODEL_MIN IMU_MODEL_ICM20948
typedef enum IMUModel {
    IMU_MODEL_NONE,
    IMU_MODEL_ICM20948,
} IMUModel;
#define IMU_MODEL_MAX IMU_MODEL_ICM20948

typedef enum IMUAxis {
    IMU_AXIS_NONE,
    IMU_AXIS_ROLL,
    IMU_AXIS_PITCH,
    IMU_AXIS_YAW,
} IMUAxis;

#define BARO_MODEL_MIN BARO_MODEL_NONE // No barometer is a valid configuration
typedef enum BaroModel {
    BARO_MODEL_NONE,
    BARO_MODEL_DPS310,
} BaroModel;
#define BARO_MODEL_MAX BARO_MODEL_DPS310

typedef bool (*aahrs_init_t)();
typedef void (*aahrs_deinit_t)();
typedef void (*aahrs_update_t)();
typedef bool (*aahrs_calibrate_t)();

// Altitude-Attitude Heading Reference System (AAHRS)
typedef struct AAHRS {
    f32 roll, pitch, yaw;             // (Read-only), deg
    f32 rollRate, pitchRate, yawRate; // (Read-only), deg/s
    // Note that while roll, pitch, and yaw are guaranteed to be abstracted by AAHRS to indicate the correct axes,
    // accelerations are not. This means that the directions of X, Y, and Z can very between aircraft.
    f32 accel[3];       // [X, Y, Z] (Read-only), g
    f32 alt;            // (Read-only)
    bool isCalibrated;  // (Read-only)
    bool isInitialized; // (Read-only)
    /**
     * Initializes the AAHRS computation layer, sensor hardware, and underlying fusion algorithms.
     * @return true if successful, false if not.
     */
    aahrs_init_t init;
    /**
     * Deinitializes and stops the AAHRS system.
     */
    aahrs_deinit_t deinit;
    /**
     * Polls sensors for updated data and periodically runs the AAHRS fusion algorithm when applicable.
     * @note This function must be called as often as possible to obtain many sensor readings for the algorithms to work with!
     */
    aahrs_update_t update;
    /**
     * Initiates a calibration of the AAHRS system.
     * This includes relavent accelerometer, magnetometer, gyroscope, and barometer calibration.
     * @return true if successful, false if not.
     */
    aahrs_calibrate_t calibrate;
} AAHRS;

extern AAHRS aahrs;

// Obtains the difference between two angles in degrees.
#define ANGLE_DIFFERENCE(a1, a2)                                                                                               \
    ((a2 - a1 + 180) % 360 - 180) < -180 ? ((a2 - a1 + 180) % 360 - 180) + 360 : ((a2 - a1 + 180) % 360 - 180)
