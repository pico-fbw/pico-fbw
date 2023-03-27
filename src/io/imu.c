#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "imu.h"
#include "../config.h"

/**
 * Initializes the IMU unit.
 * @return 0 if success, 1 if failure.
*/
int imuInit() {
    #ifndef IMU_ENABLE
      #warning IMU features are disabled, was this intentional?
      return 1;
    #endif

    #ifdef BNO055
      // scan for i2c on buses, if device found, check id and return 0
    #else
      #ifdef IMU_ENABLE
        #error No IMU module was defined. Please define an IMU module to use or disable IMU functionality.
      #endif
    #endif
}

/**
 * Configures the IMU to recieve spatial reference data.
 * @return 0 if success, 1 if failure.
*/
int imuConfigure() {
    // check calibration, if ok, set op mode to NDOF
    // maybe blink LED if something is wrong?
}