#ifndef __AAHRS_H
#define __AAHRS_H

#include <stdbool.h>

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

// Altitude-Attitude Heading Reference System (AAHRS)
typedef struct AAHRS {
    float roll;
    float pitch;
    float yaw;
    float alt;
    // Whether AAHRS should lock the opposite core when writing values
    // This should be false ONLY when it is certain AAHRS is not being accessed and critical operations must be performed!
    volatile bool lock;
    /**
     * Initializes the AAHRS computation layer, sensor hardware, and underlying fusion algorithms.
     * @return true if successful, false if not.
    */
    aahrs_init_t init;
    /**
     * Deinitializes and stops the AAHRS system.
    */
    aahrs_deinit_t deinit;
} AAHRS;

extern AAHRS aahrs;

/**
 * Obtains the difference between two angles in degrees.
*/
#define ANGLE_DIFFERENCE(a1, a2) \
    ((a2 - a1 + 180) % 360 - 180) < -180 ? ((a2 - a1 + 180) % 360 - 180) + 360 : ((a2 - a1 + 180) % 360 - 180)

#endif // __AAHRS_H