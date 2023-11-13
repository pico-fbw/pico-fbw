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

#define BARO_MODEL_MIN BARO_MODEL_NONE // No barometer is a valid configuration
typedef enum BaroModel {
    BARO_MODEL_NONE,
    BARO_MODEL_DPS310
} BaroModel;
#define BARO_MODEL_MAX BARO_MODEL_DPS310

// Altitude-Attitude Heading Reference System (AAHRS)
typedef struct AAHRS {
    float roll;
    float pitch;
    float yaw;
    float alt;
    // Whether AAHRS should lock the opposite core when writing values
    // This should be false ONLY when it is certain AAHRS is not being accessed and critical operations must be performed!
    volatile bool lock;
} AAHRS;

extern AAHRS aahrs;

/**
 * Initializes the AAHRS computation layer, sensor hardware, and underlying fusion algorithms.
 * @return true if successful, false if not.
*/
bool aahrs_init();

/**
 * Deinitializes and stops the AAHRS system.
*/
void aahrs_deinit();

#endif // __AAHRS_H