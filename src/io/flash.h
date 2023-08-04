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
 * FLASHMAP:
 * "Sector"  |  Use
 *           |
 * 0         |  PWM calibration flag / data
 *           |  0 - Flag
 *           |  1 - Pin 0 offset
 *           |  2 - Pin 1 offset
 *           |  3 - Pin 2 offset
 *           |  4 - Pin 3 offset
 * 
 * 1         |  PID tuning flag / data
 *           |  0 - Flag
 *           |  1 - Roll kP
 *           |  2 - Roll tI
 *           |  3 - Roll tD
 * 
 * 2         |  PID tuning flag / data
 *           |  0 - Flag
 *           |  1 - Pitch kP
 *           |  2 - Pitch tI
 *           |  3 - Pitch tD
 * 
 * 3         |  Bootup flag
 *           |  0 - Flag
 * 
 * 4         |  IMU axis mapping and direction flag / data
 *           |  0 - Flag
 *           |  1 - X axis map
 *           |  2 - Y axis map
 *           |  3 - Z axis map
 *           |  4 - X axis direction
 *           |  5 - Y axis direction
 *           |  6 - Z axis direction
*/

#define FLASH_MIN_SECTOR FLASH_SECTOR_PWM
typedef enum FlashSector {
    FLASH_SECTOR_PWM,
    FLASH_SECTOR_PID0,
    FLASH_SECTOR_PID1,
    FLASH_SECTOR_BOOT,
    FLASH_SECTOR_IMU
} FlashSector;
#define FLASH_MAX_SECTOR FLASH_SECTOR_IMU

// This is a fixed value so that locations of data will not change if more sectors are ever added
// It works out to give each sector 8 floats of data
#define FLASH_NUM_SECTORS 125

// The amount of floats we can fit in one flash page.
#define CONFIG_SECTOR_SIZE_FULL FLASH_SECTOR_SIZE/sizeof(float) // don't use 1024, it's buggy ~ Myles
// The amount of floats we can fit in one config sector.
#define CONFIG_SECTOR_SIZE CONFIG_SECTOR_SIZE_FULL/FLASH_NUM_SECTORS
// Size of a config sector in bytes.
#define CONFIG_SECTOR_SIZE_BYTES CONFIG_SECTOR_SIZE*sizeof(float)

#define FLASH_PHYSECTOR 0 // The physical sector that the virtual sectors are placed in
#define FLASH_PHYSECTOR_LOC (PICO_FLASH_SIZE_BYTES - (FLASH_SECTOR_SIZE * (FLASH_PHYSECTOR + 1))) // The location of the physical sector in memory

/**
 * Writes an array of data to a certain "sector".
 * This function requires the data to be structured as a float array.
 * This WILL erase and overwrite ALL data stored in the given sector!
 * @param sector the "sector" to write to
 * @param data pointer to array of data to write (must be a float array with maximum size of CONFIG_SECTOR_SIZE)
*/
void flash_write(FlashSector sector, float *data);

/**
 * Reads back one value from a previously written data array.
 * @param sector the "sector" to read from
 * @param val the value of data to read back. This should be the index of the same value from when you originally wrote the data.
 * @return the requested data according to the parameters.
 * @note This function only does a bit of math to figure out where your requested data SHOULD BE; it will read read garbage data,
 * program data, or no data at all. Be careful!
*/
float flash_read(FlashSector sector, uint val);

/**
 * Erases only the flash sector that the program actually uses.
*/
void flash_reset();

#define FBW_BOOT 3.1305210f // DO NOT CHANGE THIS VALUE! IT WILL BRICK ALL SYSTEMS!!!

#endif // __FLASH_H
