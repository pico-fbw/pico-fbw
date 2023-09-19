#ifndef __FLASH_H
#define __FLASH_H

#include "hardware/flash.h"

/**
 * README:
 * Some important info, because if you've gotten this far, you're probably wondering what a "sector" is and why it's in quotes everywhere.
 * I've created 125 virtual "sectors" that can store 8 floats each, which all fit inside the same physical sector on the flash memory (the last one).
 * Reading/writing to/from the virtual sectors is very different than writing to the physical sectors, so be warned!!
*/

/**
 * FLASHMAP (floats):
 * "Sector"  |  Use
 * 
 * 0         |  Boot
 *           |  0 - Flag
 *           |  1 - unused
 *           |  2 - unused
 *           |  3 - unused
 *           |  4 - unused
 *           |  5 - unused
 *           |  6 - unused
 *           |  7 - unused
 * 
 * 1         |  PWM offset calibration flag / data
 *           |  0 - Flag
 *           |  1 - INPUT_AIL_PIN offset
 *           |  2 - INPUT_ELEV_PIN offset
 *           |  3 - INPUT_RUD_PIN offset
 *           |  4 - INPUT_SW_PIN offset
 *           |  5 - INPUT_THR_PIN offset
 *           |  6 - Control mode flag
 *           |  7 - unused
 * 
 * 2         |  IMU axis mapping and direction flag / data
 *           |  0 - Flag
 *           |  1 - X axis map
 *           |  2 - Y axis map
 *           |  3 - Z axis map
 *           |  4 - X axis direction
 *           |  5 - Y axis direction
 *           |  6 - Z axis direction
 *           |  7 - unused
 * 
 * 3         |  IMU calibration profile flag / data (1)
 *           |  0 - Flag
 *           |  1 - Accelerometer offset X
 *           |  2 - Accelerometer offset Y
 *           |  3 - Accelerometer offset Z
 *           |  4 - Magnetometer offset X
 *           |  5 - Magnetometer offset Y
 *           |  6 - Magnetometer offset Z
 *           |  7 - unused
 * 
 * 4         |  IMU calibration profile data (2)
 *           |  0 - Gyroscope offset X
 *           |  1 - Gyroscope offset Y
 *           |  2 - Gyroscope offset Z
 *           |  3 - Accelerometer radius
 *           |  4 - Magnetometer radius
 * 
 * 5         |  PID tuning flag / data
 *           |  0 - Flag
 *           |  1 - Roll kP
 *           |  2 - Roll tI
 *           |  3 - Roll tD
 *           |  4 - Pitch kP
 *           |  5 - Pitch tI
 *           |  6 - Pitch tD
 *           |  7 - unused
*/

#define FLOAT_SECTOR_MIN FLOAT_SECTOR_PWM
typedef enum FloatSector {
    FLOAT_SECTOR_BOOT,
    FLOAT_SECTOR_PWM,
    FLOAT_SECTOR_IMU_MAP,
    FLOAT_SECTOR_IMU_CFG0,
    FLOAT_SECTOR_IMU_CFG1,
    FLOAT_SECTOR_PID
} FloatSector;
#define FLOAT_SECTOR_MAX FLOAT_SECTOR_PID

// Gets the memory location of a given PHYSICAL (not virtual!) sector.
#define GET_PHYSECTOR_LOC(sector) (PICO_FLASH_SIZE_BYTES - (FLASH_SECTOR_SIZE * (sector + 1)))

// Each "sector" has a max of 8 floats
// This is a fixed value so that locations of data will not change if more sectors are ever added
#define FLASH_NUM_FLOAT_SECTORS 125
// The amount of floats that can be fit in one flash page
#define FLOAT_SECTOR_SIZE_FULL FLASH_SECTOR_SIZE/sizeof(float) // don't use 1024, it's buggy ~ Myles
// The amount of floats that can be fit in one float sector
#define FLOAT_SECTOR_SIZE FLOAT_SECTOR_SIZE_FULL/FLASH_NUM_FLOAT_SECTORS
#define FLOAT_SECTOR_SIZE_BYTES FLOAT_SECTOR_SIZE*sizeof(float)
// The physical sector that the virtual float "sectors" are placed in
#define FLOAT_PHYSECTOR 0
#define FLOAT_PHYSECTOR_LOC GET_PHYSECTOR_LOC(FLOAT_PHYSECTOR)

#define STRING_SECTOR_MIN STRING_SECTOR_VERSION
typedef enum StringSector {
    STRING_SECTOR_VERSION,
    STRING_SECTOR_WIFLY_NETNAME,
    STRING_SECTOR_WIFLY_PASS
} StringSector;
#define STRING_SECTOR_MAX STRING_SECTOR_WIFLY_PASS

// Each "sector" has a max length of 64 characters
#define FLASH_NUM_STRING_SECTORS 16
// The amount of chars that can be fit in one flash page
#define STRING_SECTOR_SIZE_FULL FLASH_SECTOR_SIZE/sizeof(char)
// The amount of chars that can be fit in one string sector
#define STRING_SECTOR_SIZE STRING_SECTOR_SIZE_FULL/FLASH_NUM_STRING_SECTORS
#define STRING_SECTOR_SIZE_BYTES STRING_SECTOR_SIZE*sizeof(char)
// The physical sector that the virtual string "sectors" are placed in
#define STRING_PHYSECTOR 1
#define STRING_PHYSECTOR_LOC GET_PHYSECTOR_LOC(STRING_PHYSECTOR)

/**
 * Erases only the flash sectors that the program actually uses.
*/
void flash_reset();

/**
 * Writes an array of float data to a certain "sector".
 * This function requires the data to be structured as a float array with size of CONFIG_SECTOR_SIZE.
 * This WILL overwrite ALL data stored in the given sector!
 * @param sector the "sector" to write to
 * @param data pointer to array of data to write (must be a float array with size of CONFIG_SECTOR_SIZE)
*/
void flash_writeFloat(FloatSector sector, float data[]);

/**
 * Reads back one float value from a previously written FLOAT data array.
 * @param sector the "sector" to read from
 * @param val the value of data to read back. This should be the index of the same value from when you originally wrote the data.
 * @return the requested data according to the parameters.
*/
float flash_readFloat(FloatSector sector, uint val);

/**
 * Writes a string to a certain "sector".
 * This function requires the data to be structured as a char array with size of STRING_SECTOR_SIZE.
 * This WILL overwrite ALL data stored in the given sector!
 * @param sector the "sector" to write to
 * @param data pointer to array of data to write (must be a char array with size of STRING_SECTOR_SIZE)
*/
void flash_writeString(StringSector sector, char data[]);

/**
 * Reads back a string from a previously written STRING data array.
 * @param sector the "sector" to read from
 * @return the requested data string.
 * @note This can return NULL if no data has been written yet.
*/
const char *flash_readString(StringSector sector);

// DO NOT CHANGE THESE VALUES! IT WILL BRICK/RESET SYSTEMS!!!
#define FLAG_BOOT 3.1305210f
#define FLAG_PWM 0.5f
#define FLAG_IMU 0.7f
#define FLAG_PID 0.3f

#endif // __FLASH_H
