#include "hardware/flash.h"

#ifndef flash_h
#define flash_h

// This is the size we will use for our arrays that we will write to flash--it's the amount of floats we can fit in one flash page.
#define CONFIG_SECTOR_SIZE FLASH_SECTOR_SIZE/sizeof(float)

/**
 * 
*/
void flash_write(uint sector, float data[FLASH_PAGE_SIZE/sizeof(float)]);

/**
 * 
*/
float flash_read(uint sector, uint val);

/**
 * 
*/
void flash_erase(uint sector);

#endif // flash_h