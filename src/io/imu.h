#ifndef __IMU_H
#define __IMU_H

#include <stdint.h>
#include "../config.h"

// The i2c bus that will be used for the IMU
#define IMU_I2C i2c0

// Chip-specific information
#ifdef IMU_BNO055
	#define CHIP_FREQ_KHZ 400 // Default I2C freq of the BNO055 is 400KHz

	#define CHIP_REGISTER 0x28
	static const unsigned char ID_REGISTER = 0x00;
	#define CHIP_ID 0xA0
	#define CALIBRATION_REGISTER 0x35

	#define SYS_REGISTER 0x3F
	#define SYS_RESET 0x20
	static const unsigned char OPR_MODE_REGISTER = 0x3D;
	#define MODE_NDOF 0x0C
	#define PWR_MODE_REGISTER 0x3E
	#define PWR_MODE_NORMAL 0x00
	#define AXIS_MAP_CONF_REGISTER 0x41
	#define AXIS_MAP_SIGN_REGISTER 0x42

	static const unsigned char EULER_BEGIN_REGISTER = 0x1A;
	static const unsigned char ACCEL_BEGIN_REGISTER = 0x28;
#endif
// TODO: possibly support MPU6050? it will bring down cost a lot and I do have one to test with
// https://github.com/rfetick/MPU6050_light may be of use here because we do need motion fusion

/**
 * Contains heading, roll, pitch, and yaw angles of the aircraft when filled using imu_getAngles().
*/
typedef struct inertialAngles {
    float heading;
    float roll;
    float pitch;
	float yaw;
} inertialAngles;

/**
 * Contains acceleration values on the X, Y, and Z axes when filled using imu_getAccel().
*/
typedef struct inertialAccel {
	float x;
	float y;
	float z;
} inertialAccel;

/**
 * Initializes the IMU unit.
 * @return 0 if success (correct IMU type was initialized and recognized),
 * PICO_ERROR_GENERIC if there was an I2C read/write failure,
 * PICO_ERROR_TIMEOUT if there was an I2C timeout, or
 * 1 if there was a general error
*/
int imu_init();

/**
 * Deinitalizes the IMU unit.
*/
void imu_deinit();

/**
 * Configures the IMU to send inertial reference data.
 * @return true if success, false if failure.
*/
bool imu_configure();

/**
 * Gets the current angles of spatial orientation from the IMU.
 * @return an inertialAngles struct containing heading, roll, and pitch data.
*/
inertialAngles imu_getAngles();

/**
 * Gets the current acceleration values from the IMU.
 * @return an intertialAccel struct containing acceleration data for the X, Y, and Z axes.
*/
inertialAccel imu_getAccel();

/**
 * Changes the working mode of the IMU.
 * @param mode The code of the mode to change into (for example, 0x0C for NDOF).
 * @return true if success, false if failure.
*/
bool imu_changeMode(uint8_t mode);

#endif // __IMU_H
