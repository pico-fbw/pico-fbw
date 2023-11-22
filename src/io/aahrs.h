#ifndef __AAHRS_H
#define __AAHRS_H

#include <stdbool.h>

// The number of samples to average when calculating the gyroscope offset
#define GYRO_AVG_SAMPLES 100
// The maximum velocity (in deg/s) under which the gyroscope is considered to be stationary
#define GYRO_STILL_VELOCITY 3
// The maximum number of attempts to get a good (< 3.5%) magnetometer calibration, calibration will fail above this
#define MAX_MAG_ATTEMPTS 10

#define IMU_MODEL_MIN IMU_MODEL_BNO055
typedef enum IMUModel {
    IMU_MODEL_NONE,
    IMU_MODEL_BNO055,
    IMU_MODEL_ICM20948
} IMUModel;
#define IMU_MODEL_MAX IMU_MODEL_ICM20948

typedef enum IMUAxis {
    IMU_AXIS_NONE,
    IMU_AXIS_ROLL,
    IMU_AXIS_PITCH,
    IMU_AXIS_YAW
} IMUAxis;

#define BARO_MODEL_MIN BARO_MODEL_NONE // No barometer is a valid configuration
typedef enum BaroModel {
    BARO_MODEL_NONE,
    BARO_MODEL_DPS310
} BaroModel;
#define BARO_MODEL_MAX BARO_MODEL_DPS310

typedef bool (*aahrs_init_t)();
typedef void (*aahrs_deinit_t)();
typedef void (*aahrs_update_t)();
typedef bool (*aahrs_calibrate_t)();
typedef bool (*aahrs_isCalibrated_t)();

// Altitude-Attitude Heading Reference System (AAHRS)
typedef struct AAHRS {
    float roll;
    float roll_rate;
    float pitch;
    float pitch_rate;
    float yaw;
    float yaw_rate;
    float alt;
    bool isCalibrated;
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
#define ANGLE_DIFFERENCE(a1, a2) \
    ((a2 - a1 + 180) % 360 - 180) < -180 ? ((a2 - a1 + 180) % 360 - 180) + 360 : ((a2 - a1 + 180) % 360 - 180)

// Prints a vector (1-dimensional array)
#define PRINT_VECTOR(vector) \
    do { \
        for (uint i = 0; i < count_of(vector); i++) { \
            printf("%f ", vector[i]); \
        } \
        printf("\n"); \
    } while (0)

// Prints a matrix (2-dimensional array)
#define PRINT_MATRIX(matrix) \
do { \
    for (uint i = 0; i < count_of(matrix); i++) { \
        for (uint j = 0; j < count_of(matrix[0]); j++) { \
            printf("%f ", matrix[i][j]); \
        } \
        printf("\n"); \
    } \
} while (0)

#endif // __AAHRS_H