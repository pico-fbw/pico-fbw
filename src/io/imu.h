#pragma once

#include "platform/types.h"

typedef enum IMUAxis {
    IMU_AXIS_NONE,
    IMU_AXIS_ROLL,
    IMU_AXIS_PITCH,
    IMU_AXIS_YAW,
} IMUAxis;

typedef struct IMU {
    f32 roll, pitch, yaw;             // (Read-only), deg
    f32 rollRate, pitchRate, yawRate; // (Read-only), deg/s
    // Note that while roll, pitch, and yaw are guaranteed to be abstracted to indicate the correct axes,
    // accelerations are not. This means that the directions of X, Y, and Z can very between aircraft.
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
     * @return true if calibration was successful
     */
    bool (*calibrate)();
} IMU;

extern IMU imu;

// Obtains the difference between two angles in degrees.
#define ANGLE_DIFFERENCE(a1, a2)                                                                                       \
    ((a2 - a1 + 180) % 360 - 180) < -180 ? ((a2 - a1 + 180) % 360 - 180) + 360 : ((a2 - a1 + 180) % 360 - 180)
