/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pico/multicore.h"
#include "pico/platform.h"
#include "pico/time.h"
#include "pico/types.h"

#include "hardware/sync.h"

#include "aahrs.h"

#include "../sys/info.h"

#include "flash.h"

/* Default state of the flash struct (for example, holds default config values).
Locations of values are specified by the sections' respective enums in flash.h. */
Flash flash = {
    .boot = { [ 0 ... S_BOOT_HIGHEST] = 0, FLAG_END},
    .pwm = { [ 0 ... S_PWM_HIGHEST] = 0, FLAG_END},
    .pid = {0, 0, 0, 0, 0.1f, 0.001f, -50, 50, 0, 0, 0, 0.01f, 0.001f, -50, 50, 1.0f, 0.0025f, 0.001f, 0.01f, 0.001f, -50, 50, FLAG_END},
    .general = {CTRLMODE_3AXIS_ATHR, SWITCH_TYPE_3_POS, 20, 50, 50, true, WIFLY_ENABLED_PASS, false, FLAG_END},
    .control = {0.00075f, 1.5f, 2, 10, 75, 90, 10, 180, 0, 33, 67, -15, 30, 25, 15, 20, 20, 0.5f, 1, 1, FLAG_END},
    .pins = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 2, 4, 16, 17, 21, 20, false, false, false, FLAG_END},
    .sensors = {IMU_MODEL_BNO055, BARO_MODEL_NONE, GPS_COMMAND_TYPE_PMTK, 9600, FLAG_END},
    .system = {true, true, false, false, false, false, false, 4000, FLAG_END},
    .version = "",
    .wifly_ssid = "pico-fbw",
    .wifly_pass = "wiflyfbw"
};

PrintDefs print;

static void flash_erase() {
    uint32_t offset = GET_PHYSECTOR_LOC(FLOAT_PHYSECTOR);
    uint32_t intr = save_and_disable_interrupts();
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
    offset = GET_PHYSECTOR_LOC(STRING_PHYSECTOR);
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
    restore_interrupts(intr);
}

void flash_save() {
    // Convert struct data to writable byte array
    float floats[FLOAT_SECTOR_SIZE_FULL];
    memcpy(floats, flash.boot, SIZEOF_FLOAT_SECTORS_BYTES);
    char strings[STRING_SECTOR_SIZE_FULL];
    memcpy(strings, flash.version, SIZEOF_STRING_SECTORS_BYTES);
    // Write data to flash
    aahrs.lock = false; // Request AAHRS to not lock while we're writing
    sleep_us(5000);
    // Erase all data first because science
    flash_erase();
    uint32_t offset = GET_PHYSECTOR_LOC(FLOAT_PHYSECTOR);
    uint32_t intr = save_and_disable_interrupts();
    flash_range_program(offset, (uint8_t*)floats, FLASH_SECTOR_SIZE);
    offset = GET_PHYSECTOR_LOC(STRING_PHYSECTOR);
    flash_range_program(offset, (uint8_t*)strings, FLASH_SECTOR_SIZE);
    restore_interrupts(intr);
    aahrs.lock = true;
}

uint flash_load() {
    read: {
        float *floats = (float*)GET_PHYSECTOR_LOC_ABSOLUTE(FLOAT_PHYSECTOR);
        char *strings = (char*)GET_PHYSECTOR_LOC_ABSOLUTE(STRING_PHYSECTOR);
        if (floats[0] != FLAG_BOOT) {
            // First boot, struct contains default value so write those to flash
            printf("[flash] boot flag not detected, formatting flash now...\n");
            flash.boot[0] = FLAG_BOOT;
            flash_save();
            goto read;
        }
        // Flash contains data, copy to RAM-based struct
        memcpy(flash.boot, floats, SIZEOF_FLOAT_SECTORS_BYTES);
        memcpy(flash.version, strings, SIZEOF_STRING_SECTORS_BYTES);
        // Fill in print struct
        print.fbw = (bool)flash.system[SYSTEM_DEBUG_FBW];
        print.aahrs = (bool)flash.system[SYSTEM_DEBUG_AAHRS];
        print.gps = (bool)flash.system[SYSTEM_DEBUG_GPS];
        print.wifly = (bool)flash.system[SYSTEM_DEBUG_WIFLY];
        print.network = (bool)flash.system[SYSTEM_DEBUG_NETWORK];
        print.dumpNetwork = (bool)flash.system[SYSTEM_DUMP_NETWORK];
        // Fill in build type
        #if DEBUG_BUILD
            flash.system[SYSTEM_DEBUG] = true;
        #else
            flash.system[SYSTEM_DEBUG] = false;
        #endif
        return SIZEOF_FLOAT_SECTORS_BYTES + SIZEOF_STRING_SECTORS_BYTES;
    }
}

void flash_format() {
    flash.boot[BOOT_FLAG] = 0; // Corrupt the boot flag so the system will format upon the next boot
    flash_save();
}
