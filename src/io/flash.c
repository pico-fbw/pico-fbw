/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "pico/types.h"

#include "hardware/flash.h"
#include "hardware/sync.h"

#include "flash.h"

/**
 * Erases a given PHYSICAL sector of flash.
 * @param sector the memory location of the sector to erase.
*/
static void flash_erase(uint sector) {
    uint32_t offset = sector;
    uint32_t intr = save_and_disable_interrupts();
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
    restore_interrupts(intr);
}

void flash_reset() {
    flash_erase(GET_PHYSECTOR_LOC(FLOAT_PHYSECTOR));
    flash_erase(GET_PHYSECTOR_LOC(STRING_PHYSECTOR));
}

void flash_writeFloat(FloatSector sector, float data[]) {
    // Obtain previous physical sector data to ensure we don't overwrite it
    float *old_data = (float*)(XIP_BASE + (GET_PHYSECTOR_LOC(FLOAT_PHYSECTOR)));
    // Integrate old and new data to form the new physical sector data from our virtual "sectors"
    // Start by copying old data over
    float write_data[FLOAT_SECTOR_SIZE_FULL];
    for (uint i = 0; i < FLOAT_SECTOR_SIZE_FULL; i++) {
        write_data[i] = old_data[i];
    }
    // Overwrite new data section
    uint dataIndex = 0;
    for (uint i = (FLOAT_SECTOR_SIZE * sector); i < (FLOAT_SECTOR_SIZE * (sector + 1)); i++) {
        write_data[i] = data[dataIndex];
        dataIndex++;
    }
    // Erase sector before writing to it because s c i e n c e
    flash_erase(GET_PHYSECTOR_LOC(FLOAT_PHYSECTOR));
    // Create memory offset to write into
    uint32_t offset = GET_PHYSECTOR_LOC(FLOAT_PHYSECTOR);
    // Disable interrupts because they can mess with writing
    uint32_t intr = save_and_disable_interrupts();
    // Write data (cast to bytes because that's the only type you can write to flash)
    flash_range_program(offset, (uint8_t*)write_data, FLASH_SECTOR_SIZE);
    restore_interrupts(intr);
}

float flash_readFloat(FloatSector sector, uint val) {
    // Move the pointer forward to whatever address holds the requested value and return it
    return *(float*)(XIP_BASE + (GET_PHYSECTOR_LOC(FLOAT_PHYSECTOR) + (FLOAT_SECTOR_SIZE_BYTES * sector) + (sizeof(float) * val)));
}

void flash_writeString(StringSector sector, char data[]) {
    char *old_data = (char*)(XIP_BASE + (GET_PHYSECTOR_LOC(STRING_PHYSECTOR)));
    char write_data[STRING_SECTOR_SIZE_FULL];
    for (uint i = 0; i < STRING_SECTOR_SIZE_FULL; i++) {
        write_data[i] = old_data[i];
    }
    uint dataIndex = 0;
    for (uint i = (STRING_SECTOR_SIZE * sector); i < (STRING_SECTOR_SIZE * (sector + 1)); i++) {
        write_data[i] = data[dataIndex];
        dataIndex++;
    }
    flash_erase(GET_PHYSECTOR_LOC(STRING_PHYSECTOR));
    uint32_t offset = GET_PHYSECTOR_LOC(STRING_PHYSECTOR);
    uint32_t intr = save_and_disable_interrupts();
    flash_range_program(offset, (uint8_t*)write_data, FLASH_SECTOR_SIZE);
    restore_interrupts(intr);
}

char *flash_readString(StringSector sector) { return (char*)(XIP_BASE + (GET_PHYSECTOR_LOC(STRING_PHYSECTOR) + (STRING_SECTOR_SIZE_BYTES * sector))); }
