#include "../config.h"

#ifdef IMU_BNO055
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

  static const uint8_t EULER_REGISTER = 0x1A;
#endif

/**
 * Initializes the IMU unit.
 * @return 0 if success (correct IMU type was recognized), PICO_ERROR_GENERIC if there was an I2C read/write failure, or 1 if there was a general error
*/
int imuInit();

/**
 * Configures the IMU to send spatial reference data.
 * @return 0 if success, 1 if failure.
*/
int imuConfigure();