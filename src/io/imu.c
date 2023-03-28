#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

#include "imu.h"
#include "../config.h"

int imuInit() {
    // Check if the IMU is NOT enabled, if so, immediately throw an error (preprocessor error as well)
    #ifndef IMU_ENABLE
      #warning IMU features are disabled, was this intentional?
      return 1;
    #endif
    // Check if the BNO055 unit is defined, if so,
    #ifdef IMU_BNO055
      // Check for default i2c constants
      #if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
        #warning No I2C defaults found, functionality may be impacted.
      #endif
      // Scan for i2c on pins 4 and 5, if device found, check id w/ what BNO055 should be and return 0
      printf("initializing i2c connection\n");
      i2c_init(i2c_default, 50000);
      gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
      gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
      gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
      gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
      bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

      printf("querying chip for ID\n");
      uint8_t id;
      int result = i2c_write_blocking(i2c_default, CHIP_REGISTER, &ID_REGISTER, 1, true);
      if (result != PICO_ERROR_GENERIC) {
          result = i2c_read_blocking(i2c_default, CHIP_REGISTER, &id, 1, false);
          if (result != PICO_ERROR_GENERIC) {
              if (id == CHIP_ID) {
                  printf("success! id recieved was: %d\n", id);
                  return 0;
              } else {
                  printf("unexpected response\n");
                  return 1;
              }
          } else {
            printf("i2c read error\n");
            return PICO_ERROR_GENERIC;
          }
      } else {
        printf("i2c write error\n");
        return PICO_ERROR_GENERIC;
      }
    #else
      // If no BNO055 is defined, check if IMU is enabled, if so, stop compilation b/c that is an invalid combination (but IMU not enabled with no IMU selected is fine)
      #ifdef IMU_ENABLE
        #error No IMU module was defined. Please define an IMU module to use or disable IMU functionality.
      #endif
    #endif
}

int imuConfigure() {
    // reset
    printf("resetting chip\n");
    uint8_t reset_cmd[2] = {SYS_REGISTER, SYS_RESET};
    i2c_write_blocking(i2c_default, CHIP_REGISTER, reset_cmd, 2, true);
    sleep_ms(850);
    printf("setting normal pwr mode\n");

    // set to normal power mode
    i2c_write_blocking(i2c_default, CHIP_REGISTER, &PWR_MODE_REGISTER, 1, true);
    i2c_write_blocking(i2c_default, CHIP_REGISTER, &PWR_MODE_NORMAL, 1, false);
    sleep_ms(100);
    uint8_t pwr;
    i2c_write_blocking(i2c_default, CHIP_REGISTER, &PWR_MODE_REGISTER, 1, true);
    i2c_read_blocking(i2c_default, CHIP_REGISTER, &pwr, 1, false);
    printf("pwr mode is currently %d\n", pwr);

    // temp test
    uint8_t temp_reg = 0x34; // TEMP_REG address
    int8_t temp_value; // temperature value (signed 1-byte integer)
    // write the TEMP_REG address to the BNO055
    i2c_write_blocking(i2c_default, CHIP_REGISTER, &temp_reg, 1, true);
    // read the temperature value from the BNO055
    i2c_read_blocking(i2c_default, CHIP_REGISTER, &temp_value, 1, false);
    // print the temperature value in degrees Celsius
    printf("Chip temperature: %d degrees Celsius\n", temp_value);

    // set to NDOF mode
    printf("setting NDOF mode\n");
    i2c_write_blocking(i2c_default, CHIP_REGISTER, &OPR_MODE_REGISTER, 1, true);
    i2c_write_blocking(i2c_default, CHIP_REGISTER, &MODE_NDOF, 1, false);
    sleep_ms(100); // wait for the sensor to change modes
    uint8_t mode;
    i2c_write_blocking(i2c_default, CHIP_REGISTER, &OPR_MODE_REGISTER, 1, true);
    i2c_read_blocking(i2c_default, CHIP_REGISTER, &mode, 1, false);
    if (mode == MODE_NDOF) {
      printf("mode change success!\n");
      printf("mode: %d\n", mode);
      return 0;
    } else {
      printf("mode change failure\n");
      printf("mode: %d\n", mode);
      return 1;
    }
}