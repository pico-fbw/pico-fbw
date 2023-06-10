#include "hardware/flash.h"

#ifndef flash_h
#define flash_h

/**
 * README:
 * Some important info, because if you've gotten this far, you're probably wondering what a "sector" is and why it's in quotes everywhere.
 * The reason is, my implementation of flash is based on real flash sectors (it has to be), but they aren't truly sectors,
 * at least in terms of starting from the beginning.
 * Sector 0 is the last sector of flash, sector 1 is the second-to-last, etc.
 * I implemented in this way so we don't have to change the flash code if the binary gets larger, even though it can be a bit confusing.
*/

/**
 * FLASHMAP:
 * Sector  |  Use
 *         |
 * 0       |  PWM calibration flag / data
 * 1       |  PID tuning flag / data
 * 2       |  PID tuning flag / data
 * 3       |  Bootup flag / versioncode
*/

// This is the size we will use for our arrays that we will write to flash--it's the amount of floats we can fit in one flash page.
#define CONFIG_SECTOR_SIZE FLASH_SECTOR_SIZE/sizeof(float)

#define FBW_BOOT 3.1305210f // DO NOT CHANGE THIS VALUE! IT WILL BRICK ALL SYSTEMS!!!

/**
 * Writes an array of data to a certain "sector". Note that this function assumes the data is a float array.
 * @param sector the "sector" to write to
 * @param data the array of data to write 
*/
void flash_write(uint sector, float data[]);

/**
 * Reads back one value from a previously written data array.
 * @param sector the "sector" to read from
 * @param val the value of data to read back. This should be the index of the same value from when you originally wrote the data.
 * @return the requested data according to the parameters.
 * Do note that this function only does a bit of math to figure out where your requested data is; it will read read garbage data,
 * program data, or no data at all. Be careful!
*/
float flash_read(uint sector, uint val);

/**
 * Erases a given sector of flash.
 * @param sector the "sector" to erase.
*/
void flash_erase(uint sector);

/**
 * Erases only the flash sectors that the program actually uses.
 * This is so we don't waste flash cycles by clearing the entire flash.
*/
void flash_reset();

#endif // flash_h
