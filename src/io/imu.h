#ifndef __IMU_H
#define __IMU_H

#include "../config.h"

// The i2c bus that will be used for the IMU
#define IMU_I2C i2c0

// The time (in microseconds) before the IMU is considered unresponsive
#define IMU_TIMEOUT_US 5000

// For axis mappings in configuration file
#define ROLL_AXIS 0
#define PITCH_AXIS 1
#define YAW_AXIS 2
// Default axis mappings
#ifndef IMU_MAP_AXES
	#if defined(IMU_BNO055)
		#define IMU_X_AXIS YAW_AXIS
		#define IMU_Y_AXIS ROLL_AXIS
		#define IMU_Z_AXIS PITCH_AXIS
	#elif defined(IMU_MPU6050)
		#define IMU_X_AXIS YAW_AXIS
		#define IMU_Y_AXIS PITCH_AXIS
		#define IMU_Z_AXIS ROLL_AXIS
	#endif
#endif

// Chip-specific information
// CHIP_FREQ_KHZ, CHIP_REGISTER, ID_REGISTER, and CHIP_ID are required for all chips, the rest is usually specific to each chip
#if defined(IMU_BNO055)
	
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

	#define CHIP_FREQ_KHZ 400

	#define CHIP_REGISTER 0x68
	static const unsigned char ID_REGISTER = 0x75;
	#define CHIP_ID 0x68

	#define SMPLRT_DIV_REGISTER 0x19
	#define CONFIG_REGISTER 0x1A
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
 * Gets the current angles of spatial orientation from the IMU.
 * @return an inertialAngles struct containing heading, roll, pitch, and yaw data.
*/
inertialAngles imu_getAngles();

#endif // __IMU_H
