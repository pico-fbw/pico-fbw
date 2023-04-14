#include "hardware/flash.h"
#include "hardware/sync.h"

#include "flash.h"

void flash_write(uint sector, float data[CONFIG_SECTOR_SIZE]) {
    // Create memory offset based on the inputted "sector"
    uint32_t offset = (PICO_FLASH_SIZE_BYTES - (FLASH_SECTOR_SIZE + sector * FLASH_SECTOR_SIZE));
    // Disable interrupts because they can mess with our writing
    uint32_t intr = save_and_disable_interrupts();
    // Write our data (cast to an 8bit uint because that's the only type you can write to flash) and then restore interrupts
    flash_range_program(offset, (uint8_t *) (data), FLASH_PAGE_SIZE);
    restore_interrupts (intr);
}

float flash_read(uint sector, uint val) {
    // Move the pointer forward to whatever address holds the requested value
    float* calibration_val_ptr = (float*) ((FLASH_SECTOR_SIZE + sector * FLASH_SECTOR_SIZE) + (sizeof(float) * val));
    // Retrieve and return the actual (dereferenced) calibration value
    return *calibration_val_ptr;
}

void flash_erase(uint sector) {
    uint32_t offset = (PICO_FLASH_SIZE_BYTES - (FLASH_SECTOR_SIZE + sector * FLASH_SECTOR_SIZE));
    uint32_t intr = save_and_disable_interrupts();
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
    restore_interrupts (intr);
}
