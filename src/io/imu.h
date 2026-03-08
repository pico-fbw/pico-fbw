#pragma once

#include "platform/types.h"

typedef enum IMUAxis {
    IMU_AXIS_NONE,
    IMU_AXIS_ROLL,
    IMU_AXIS_PITCH,
    IMU_AXIS_YAW,
} IMUAxis;

typedef enum IMUCalibrationStatus {
    IMU_CALIBRATION_NOT_STARTED,
    IMU_CALIBRATION_STATIONARY,
    IMU_CALIBRATION_ROLL_MAPPING,
    IMU_CALIBRATION_PITCH_MAPPING,
    IMU_CALIBRATION_YAW_MAPPING,
    IMU_CALIBRATION_APPLYING,
    IMU_CALIBRATION_DONE,
    IMU_CALIBRATION_FAILED,
} IMUCalibrationStatus;

typedef struct IMU {
    f32 roll, pitch, yaw;             // (Read-only), deg
    f32 rollRate, pitchRate, yawRate; // (Read-only), deg/s
    // When calibrated, acceleration axes are permuted into roll/pitch/yaw axis order.
    // Sign conventions are applied to fused outputs, not raw accel.
    f32 accel[3];      // [X, Y, Z] (Read-only), g
    f32 alt;           // (Read-only), ft (MSL)
    bool ready;        // (Read-only), true if the IMU is ready to be used
    bool isCalibrated; // (Read-only), true if the IMU has been calibrated
    /**
     * Initializes the IMU computation layer and scans for available sensors.
     * @return true if successful
     */
    bool (*init)();
    /**
     * Deinitializes and stops the IMU system.
     */
    void (*deinit)();
    /**
     * Polls sensors for updated data.
     */
    void (*update)();
    /**
     * Initiates the calibration process for the IMU.
     * @return current status of the calibration process
     * @note This function is designed to be called repeatedly during the calibration process, returning the current
     * status to aid in the process. When complete, it will return `IMU_CALIBRATION_DONE` if successful or
     * `IMU_CALIBRATION_FAILED` if it fails.
     */
    IMUCalibrationStatus (*calibrate)();
} IMU;

extern IMU imu;

// Obtains the difference between two angles in degrees.
#define ANGLE_DIFFERENCE(a1, a2)                                                                                       \
    ((a2 - a1 + 180) % 360 - 180) < -180 ? ((a2 - a1 + 180) % 360 - 180) + 360 : ((a2 - a1 + 180) % 360 - 180)
