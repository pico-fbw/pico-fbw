#ifndef __IMU_H
#define __IMU_H

typedef enum IMUModel {
    IMU_MODEL_NONE,
    IMU_MODEL_BNO055
} IMUModel;

#define IMU_I2C i2c0 // The i2c bus that will be used for the IMU, do not change this!!
#define IMU_TIMEOUT_US 50000 // The maximum time to wait for a response from the IMU in microseconds

// Chip-specific information:
// CHIP_FREQ_KHZ, CHIP_REGISTER, ID_REGISTER, and CHIP_ID are required for all chips, the rest is usually specific to each chip

/* 
   BNO055 
   Datasheet: https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bno055-ds000.pdf
*/

#define BNO_CHIP_FREQ_KHZ 400

static const unsigned char BNO_CHIP_REGISTER = 0x28;
static const unsigned char BNO_ID_REGISTER = 0x00;
static const unsigned char BNO_CHIP_ID = 0xA0;

static const unsigned char CALIBRATION_REGISTER = 0x35;
static const unsigned char SYS_REGISTER = 0x3F;
static const unsigned char UNIT_SEL_REGISTER = 0x3B;
static const unsigned char OPR_MODE_REGISTER = 0x3D;
static const unsigned char PWR_MODE_REGISTER = 0x3E;
static const unsigned char AXIS_MAP_CONF_REGISTER = 0x41;
static const unsigned char AXIS_MAP_SIGN_REGISTER = 0x42;

static const unsigned char ACCEL_OFFSET_X_BEGIN_REGISTER = 0x55;
static const unsigned char GYRO_OFFSET_X_BEGIN_REGISTER = 0x61;

static const unsigned char ACC_BEGIN_REGISTER = 0x08;
static const unsigned char EULER_BEGIN_REGISTER = 0x1A;

static const unsigned char MODE_CONFIG = 0x00;
static const unsigned char MODE_AMG = 0x07;
static const unsigned char MODE_NDOF = 0x0C;
static const unsigned char PWR_MODE_NORMAL = 0x00;

// Contains heading, roll, pitch, and yaw angles of the aircraft when filled using imu_getAngles().
typedef struct Angles {
    float roll;
    float pitch;
	float yaw;
} Angles;

// Contains Euler angles of the aircraft when filled using imu_getRawAngles() (these are not mapped to actual aircraft angles).
typedef struct Euler {
    float x;
    float y;
    float z;
} Euler;

#define ANGLE_DIFFERENCE(a1, a2) \
    ((a2 - a1 + 180) % 360 - 180) < -180 ? ((a2 - a1 + 180) % 360 - 180) + 360 : ((a2 - a1 + 180) % 360 - 180)

/**
 * Initializes the IMU unit.
 * @return 0 if success (correct IMU type was initialized and recognized),
 * PICO_ERROR_GENERIC if there was an I2C read/write failure,
 * PICO_ERROR_TIMEOUT if there was an I2C timeout, or
 * 1 if there was a general error (likely chip exists but could not be identified)
*/
int imu_init();

/**
 * Deinitalizes the IMU unit.
*/
void imu_deinit();

/**
 * Configures the IMU to send inertial reference data.
 * @return true if success, false if failure.
 * @note This is also when the offsets are calculated/zeroed.
*/
bool imu_configure();

/**
 * Gets the current Euler angles of the aircraft.
 * @return a Euler struct containing x, y, and z Euler angles.
*/
Euler imu_getRawAngles();

/**
 * Gets the current angles of spatial orientation from the IMU.
 * @return an Angles struct containing roll, pitch, and yaw data.
 * @note This function compensates using calibration data obtained earlier.
 * Positive values indicate a right roll, pitch up, or right yaw, and negative values indicate the opposite.
*/
Angles imu_getAngles();

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
bool imu_isCalibrated();

#endif // __IMU_H
