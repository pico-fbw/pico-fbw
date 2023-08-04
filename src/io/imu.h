#ifndef __IMU_H
#define __IMU_H

#include "../config.h"

// The i2c bus that will be used for the IMU
#define IMU_I2C i2c0

// The time (in microseconds) before the IMU is considered unresponsive
#define IMU_TIMEOUT_US 10000

// The time (in microseconds) between samples of the IMU
// This is only done with synchronous IMUs like the MPU6050 and not async IMUs like the BNO055
// It will cause an interrupt!
#define IMU_SAMPLE_RATE_US 3000

// The angle thresholds for motion detection
// Useful for lower-quality IMUs like the MPU6050 that don't have great filtering
// Keep in mind this is compared every sample!
#define ANGLE_THRESHOLD_X 0.8f
#define ANGLE_THRESHOLD_Y 0.1f
#define ANGLE_THRESHOLD_Z 0.1f

// Chip-specific information
// CHIP_FREQ_KHZ, CHIP_REGISTER, ID_REGISTER, and CHIP_ID are required for all chips, the rest is usually specific to each chip
#if defined(IMU_BNO055)

	// Datasheet: https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bno055-ds000.pdf
	
	#define CHIP_FREQ_KHZ 400

	#define CHIP_REGISTER 0x28
	static const unsigned char ID_REGISTER = 0x00;
	#define CHIP_ID 0xA0

	#define CALIBRATION_REGISTER 0x35
	#define SYS_REGISTER 0x3F
	#define SYS_RESET 0x20
	static const unsigned char OPR_MODE_REGISTER = 0x3D;
	#define PWR_MODE_REGISTER 0x3E
	#define AXIS_MAP_CONF_REGISTER 0x41
	#define AXIS_MAP_SIGN_REGISTER 0x42

	static const unsigned char EULER_BEGIN_REGISTER = 0x1A;
	static const unsigned char ACCEL_BEGIN_REGISTER = 0x28;

	#define MODE_NDOF 0x0C
	#define PWR_MODE_NORMAL 0x00

#elif defined(IMU_MPU6050)

	// Datasheet: https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf

	#define CHIP_FREQ_KHZ 400

	#define CHIP_REGISTER 0x68
	static const unsigned char ID_REGISTER = 0x75;
	#define CHIP_ID 0x68

	#define SMPLRT_DIV_REGISTER 0x19
	#define CONFIG_REGISTER 0x1A
	#define INT_PIN_CFG_REGISTER 0x37
	#define GYRO_CONFIG_REGISTER 0x1B
	#define ACCEL_CONFIG_REGISTER 0x1C

	#define USER_CONTROL_REGISTER 0x6A
	#define PWR_MODE_REGISTER 0x6B

	#define INTERRUPTS_ENABLED_REGISTER 0x38
	#define FIFO_ENABLED_REGISTER 0x23

	#define DMP_PROG_START_ADDR 0x70
	static const unsigned char DMP_RA_FIFO_COUNT = 0x72;
	static const unsigned char DMP_RA_FIFO_R_W = 0x74;
	
#endif

// Contains heading, roll, pitch, and yaw angles of the aircraft when filled using imu_getAngles().
typedef struct inertialAngles {
    float roll;
    float pitch;
	float yaw;
} inertialAngles;

// Contains Euler angles of the aircraft when filled using imu_getRawAngles() (these are not mapped to actual aircraft angles).
typedef struct Euler {
    float x;
    float y;
    float z;
} Euler;

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
 * @note Note that if the MPU6050 is the selected IMU unit, this is also when its offsets are calculated.
*/
bool imu_configure();

/**
 * Gets the current Euler angles of the aircraft.
 * @return a Euler struct containing x, y, and z Euler angles.
*/
Euler imu_getRawAngles();

/**
 * Gets the current angles of spatial orientation from the IMU.
 * @return an inertialAngles struct containing roll, pitch, and yaw data.
 * @note This function compensates using calibration data obtained earlier.
 * Positive values indicate a right roll, pitch up, or right yaw, and negative values indicate the opposite.
*/
inertialAngles imu_getAngles();

/**
 * Runs the IMU calibration and saves values to flash.
 * Be aware this function WILL block until the calibration is complete (which requires user input)!
 * @return true if success, false if failure.
*/
bool imu_calibrate();

/**
 * Checks if the IMU calibration has been run before.
 * @return true if calibration has been run, false if not.
*/
bool imu_checkCalibration();

#endif // __IMU_H
