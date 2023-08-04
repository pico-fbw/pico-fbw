/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "hardware/flash.h"
#include "hardware/sync.h"

#include "flash.h"

/**
 * Erases a given PHYSICAL sector of flash.
 * @param sector the sector to erase.
*/
static void flash_erase(FlashSector sector) {
    uint32_t offset = (PICO_FLASH_SIZE_BYTES - (FLASH_SECTOR_SIZE * (sector + 1)));
    uint32_t intr = save_and_disable_interrupts();
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
    restore_interrupts(intr);
}

void flash_write(FlashSector sector, float *data) {
    // Obtain previous physical sector data to ensure we don't overwrite it
    float *old_data = (float*)(XIP_BASE + (FLASH_PHYSECTOR_LOC));
    // Integrate old and new data to form the new physical sector data from our virtual sectors
    // Start by copying old data over
    float write_data[CONFIG_SECTOR_SIZE_FULL];
    for (uint i = 0; i < CONFIG_SECTOR_SIZE_FULL; i++) {
        write_data[i] = old_data[i];
    }
    // Overwrite new data section
    uint dataIndex = 0;
    for (uint i = (CONFIG_SECTOR_SIZE * sector); i < (CONFIG_SECTOR_SIZE * (sector + 1)); i++) {
        write_data[i] = data[dataIndex];
        dataIndex++;
    }
    // Erase sector before writing to it because s c i e n c e
    flash_erase(FLASH_PHYSECTOR);
    // Create memory offset to write into
    uint32_t offset = FLASH_PHYSECTOR_LOC;
    // Disable interrupts because they can mess with our writing
    uint32_t intr = save_and_disable_interrupts();
    // Write our data (cast to an 8bit uint because that's the only type you can write to flash)
    flash_range_program(offset, (uint8_t*)write_data, FLASH_SECTOR_SIZE);
    restore_interrupts(intr);
}

float flash_read(FlashSector sector, uint val) {
    // Move the pointer forward to whatever address holds the requested value and return it
    return *(float*)(XIP_BASE + (FLASH_PHYSECTOR_LOC + (CONFIG_SECTOR_SIZE_BYTES * sector) + (sizeof(float) * val)));
}

void flash_reset() {
    flash_erase(FLASH_PHYSECTOR);
}
