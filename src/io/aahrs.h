#pragma once

#include <stdbool.h>
#include "platform/int.h"

#define IMU_MODEL_MIN IMU_MODEL_BNO055
typedef enum IMUModel {
    IMU_MODEL_NONE,
    IMU_MODEL_BNO055,
    IMU_MODEL_ICM20948,
    IMU_MODEL_MAX,
} IMUModel;

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
    BARO_MODEL_MAX,
} BaroModel;

typedef bool (*aahrs_init_t)();
typedef void (*aahrs_deinit_t)();
typedef void (*aahrs_update_t)();
typedef bool (*aahrs_calibrate_t)();

// Altitude-Attitude Heading Reference System (AAHRS)
typedef struct AAHRS {
    float roll;        // (Read-only)
    float rollRate;    // (Read-only)
    float pitch;       // (Read-only)
    float pitchRate;   // (Read-only)
    float yaw;         // (Read-only)
    float yawRate;     // (Read-only)
    float alt;         // (Read-only)
    bool isCalibrated; // (Read-only)
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
