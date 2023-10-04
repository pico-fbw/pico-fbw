/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "pico/types.h"

#include "hardware/sync.h"

#include "flash.h"

static float cachedFloats[FLOAT_SECTOR_SIZE_FULL];
static char cachedStrings[STRING_SECTOR_SIZE_FULL];

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

uint flash_cache() {
    float *float_physector = (float*)GET_PHYSECTOR_LOC_ABSOLUTE(FLOAT_PHYSECTOR);
    char *string_physector = (char*)GET_PHYSECTOR_LOC_ABSOLUTE(STRING_PHYSECTOR);
    memcpy(cachedFloats, float_physector, FLOAT_SECTOR_SIZE_FULL);
    memcpy(cachedStrings, string_physector, STRING_SECTOR_SIZE_FULL);
    return FLASH_SECTOR_SIZE * 2;
}

void flash_writeFloat(FloatSector sector, float data[], bool toFlash) {
    // Integrate old and new data to form the new physical sector data from our virtual "sectors"
    // Start by copying old data over
    float write_data[FLOAT_SECTOR_SIZE_FULL];
    memcpy(write_data, cachedFloats, FLOAT_SECTOR_SIZE_FULL);
    // Overwrite ONLY the new data sector
    uint dataIndex = 0;
    for (uint i = (FLOAT_SECTOR_SIZE * sector); i < (FLOAT_SECTOR_SIZE * (sector + 1)); i++) {
        write_data[i] = data[dataIndex];
        dataIndex++;
    }
    if (toFlash) {
        // Erase sector before writing to it because s c i e n c e
        flash_erase(GET_PHYSECTOR_LOC(FLOAT_PHYSECTOR));
        uint32_t offset = GET_PHYSECTOR_LOC(FLOAT_PHYSECTOR);
        uint32_t intr = save_and_disable_interrupts();
        flash_range_program(offset, (uint8_t*)write_data, FLASH_SECTOR_SIZE);
        restore_interrupts(intr);
    }
    // Cache the new data
    memcpy(cachedFloats, write_data, FLOAT_SECTOR_SIZE_FULL);
}

float flash_readFloat(FloatSector sector, uint val) { return cachedFloats[FLOAT_SECTOR_SIZE * sector + val]; }

void flash_writeString(StringSector sector, char data[], bool toFlash) {
    char write_data[STRING_SECTOR_SIZE_FULL];
    memcpy(write_data, cachedStrings, STRING_SECTOR_SIZE_FULL);
    uint dataIndex = 0;
    for (uint i = (STRING_SECTOR_SIZE * sector); i < (STRING_SECTOR_SIZE * (sector + 1)); i++) {
        write_data[i] = data[dataIndex];
        dataIndex++;
    }
    if (toFlash) {
        flash_erase(GET_PHYSECTOR_LOC(STRING_PHYSECTOR));
        uint32_t offset = GET_PHYSECTOR_LOC(STRING_PHYSECTOR);
        uint32_t intr = save_and_disable_interrupts();
        flash_range_program(offset, (uint8_t*)write_data, FLASH_SECTOR_SIZE);
        restore_interrupts(intr);
    }
    memcpy(cachedStrings, write_data, STRING_SECTOR_SIZE_FULL);
}

const char *flash_readString(StringSector sector) {
    const char *data = &cachedStrings[STRING_SECTOR_SIZE * sector];
    // Ensure the sector has been initialized (flash is by default initialized to 0xFF which UTF-8 and my code are not very fond of)
    if (data[0] != UINT8_MAX) {
        return data;
    } else {
        return NULL;
    }
}

void flash_flushCache() {
    // Write cache data to flash
    flash_erase(GET_PHYSECTOR_LOC(FLOAT_PHYSECTOR));
    flash_erase(GET_PHYSECTOR_LOC(STRING_PHYSECTOR));
    uint32_t offset = GET_PHYSECTOR_LOC(FLOAT_PHYSECTOR);
    uint32_t intr = save_and_disable_interrupts();
    flash_range_program(offset, (uint8_t*)cachedFloats, FLASH_SECTOR_SIZE);
    offset = GET_PHYSECTOR_LOC(STRING_PHYSECTOR);
    flash_range_program(offset, (uint8_t*)cachedStrings, FLASH_SECTOR_SIZE);
    restore_interrupts(intr);
}
