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

// TODO: allow bulk flash writes to cache and later write to flash all at once to speed up write times

static float cachedFloats[FLOAT_SECTOR_SIZE_FULL];
static char cachedStrings[STRING_SECTOR_SIZE_FULL];

#if !FLASH_MUST_CACHE
    static bool hasCached = false;
#endif

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
    #if !FLASH_MUST_CACHE
        hasCached = true;
    #endif
}

void flash_writeFloat(FloatSector sector, float data[]) {
    // Obtain previous physical sector data to ensure we don't overwrite it
    float *old_data = (float*)GET_PHYSECTOR_LOC_ABSOLUTE(FLOAT_PHYSECTOR);
    // Integrate old and new data to form the new physical sector data from our virtual "sectors"
    // Start by copying old data over
    float write_data[FLOAT_SECTOR_SIZE_FULL];
    memcpy(write_data, old_data, FLOAT_SECTOR_SIZE_FULL);
    // Overwrite ONLY the new data section
    uint dataIndex = 0;
    for (uint i = (FLOAT_SECTOR_SIZE * sector); i < (FLOAT_SECTOR_SIZE * (sector + 1)); i++) {
        write_data[i] = data[dataIndex];
        dataIndex++;
    }
    // Erase sector before writing to it because s c i e n c e (the data is backed up in RAM at the moment)
    flash_erase(GET_PHYSECTOR_LOC(FLOAT_PHYSECTOR));
    uint32_t offset = GET_PHYSECTOR_LOC(FLOAT_PHYSECTOR);
    uint32_t intr = save_and_disable_interrupts();
    flash_range_program(offset, (uint8_t*)write_data, FLASH_SECTOR_SIZE);
    restore_interrupts(intr);
    // Additionally, cache the new data
    memcpy(cachedFloats, write_data, FLOAT_SECTOR_SIZE_FULL);
}

float flash_readFloat(FloatSector sector, uint val) {
    // If strict caching is enabled...
    #if FLASH_MUST_CACHE
        // ...pull from cache directly as flash has already been cached
        goto cache;
    #else
        // If not, first check if cached data exists
        if (hasCached) {
            goto cache;
        } else {
            goto flash;
        }
    #endif
    cache:
        return cachedFloats[FLOAT_SECTOR_SIZE * sector + val];
    flash:
        return *(float*)(GET_PHYSECTOR_LOC_ABSOLUTE(FLOAT_PHYSECTOR) + (FLOAT_SECTOR_SIZE_BYTES * sector) + (sizeof(float) * val));    
}

void flash_writeString(StringSector sector, char data[]) {
    char *old_data = (char*)GET_PHYSECTOR_LOC_ABSOLUTE(STRING_PHYSECTOR);
    char write_data[STRING_SECTOR_SIZE_FULL];
    memcpy(write_data, old_data, STRING_SECTOR_SIZE_FULL);
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
    memcpy(cachedStrings, write_data, STRING_SECTOR_SIZE_FULL);
}

const char *flash_readString(StringSector sector) {
    const char *data;
    #if FLASH_MUST_CACHE
        data = &cachedStrings[STRING_SECTOR_SIZE * sector];
    #else
        if (hasCached) {
            data = &cachedStrings[STRING_SECTOR_SIZE * sector];
        } else {
            data = (const char*)(GET_PHYSECTOR_LOC_ABSOLUTE(STRING_PHYSECTOR) + (STRING_SECTOR_SIZE_BYTES * sector));
        }
    #endif
    // Ensure the sector has been initialized (flash is by default initialized to 0xFF which UTF-8 and my code are not very fond of)
    if (data[0] != UINT8_MAX) {
        return data;
    } else {
        return NULL;
    }
}
