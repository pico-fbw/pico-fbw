#include "../config.h"

#ifndef imu_h
#define imu_h

#ifdef IMU_BNO055
	static const uint CHIP_FREQ_KHZ = 400; // Default I2C freq of the BNO055 is 400KHz

	static const uint8_t CHIP_REGISTER = 0x28;
	static const uint8_t ID_REGISTER = 0x00;
	static const uint8_t CHIP_ID = 0xA0;
	static const uint8_t CALIBRATION_REGISTER = 0x35;

	static const uint8_t SYS_REGISTER = 0x3F;
	static const uint8_t SYS_RESET = 0x20;
	static const uint8_t OPR_MODE_REGISTER = 0x3D;
	static const uint8_t MODE_NDOF = 0x0C;
	static const uint8_t PWR_MODE_REGISTER = 0x3E;
	static const uint8_t PWR_MODE_NORMAL = 0x00;
	static const uint8_t AXIS_MAP_CONF_REGISTER = 0x41;
	static const uint8_t AXIS_MAP_SIGN_REGISTER = 0x42;

	static const uint8_t EULER_BEGIN_REGISTER = 0x1A;
	static const uint8_t ACCEL_BEGIN_REGISTER = 0x28;
#endif

/**
 * Initializes the IMU unit.
 * @return 0 if success (correct IMU type was initialized and recognized), PICO_ERROR_GENERIC if there was an I2C read/write failure, PICO_ERROR_TIMEOUT if there was an I2C timeout, or 1 if there was a general error
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
 * A struct containing heading, roll, and pitch angles of the aircraft (when filled using its corresponding method).
*/
typedef struct inertialAngles {
    float heading;
    float roll;
    float pitch;
} inertialAngles;

/**
 * A struct containing acceleration values on the X, Y, and Z axes (when filled using its corresponding method).
*/
typedef struct inertialAccel {
	float x;
	float y;
	float z;
} inertialAccel;

/**
 * Gets the current angles of spatial orientation from the IMU.
 * @return an inertialAngles struct containing heading, roll, and pitch data.
*/
struct inertialAngles imu_getAngles();

/**
 * Gets the current acceleration values from the IMU.
 * @return an intertialAccel struct containing acceleration data for the X, Y, and Z axes.
*/
struct inertialAccel imu_getAccel();

/**
 * Changes the working mode of the IMU.
 * @param mode The code of the mode to change into (for example, 0x0C for NDOF).
 * @return true if success, false if failure.
*/
bool imu_changeMode(uint8_t mode);

/**
 * A low(er)-level method that writes a value directly to the IMU over I2C.
 * @param address The address to write to.
 * @param value The value to write.
 * @return Number of bytes written, or PICO_ERROR_GENERIC if address not acknowledged, no device present.
*/
int imu_write(uint8_t address, uint8_t value);

#endif // imu_h
