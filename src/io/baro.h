#ifndef __BARO_H
#define __BARO_H

typedef enum BaroModel {
    BARO_MODEL_NONE,
    BARO_MODEL_DPS310
} BaroModel;

#define BARO_TIMEOUT_US 5000 // The maximum time to wait for a response from the barometer in microseconds

/*
    DPS310
    Datasheet: https://www.infineon.com/dgdl/Infineon-DPS310-DataSheet-v01_02-EN.pdf
*/

static const unsigned char DPS_CHIP_REGISTER = 0x77;
static const unsigned char DPS_ID_REGISTER = 0x0D;
static const unsigned char DPS_CHIP_ID = 0x10;

/**
 * Initializes the barometer unit.
 * @return 0 if success (correct barometer type was initialized and recognized),
 * PICO_ERROR_GENERIC if there was an I2C read/write failure,
 * PICO_ERROR_TIMEOUT if there was an I2C timeout, or
 * 1 if there was a general error (likely chip exists but could not be identified)
 * @note This function expects the barometer to be on the same i2c bus as the IMU, and for the IMU to be initialized.
*/
int baro_init();

#endif // __BARO_H