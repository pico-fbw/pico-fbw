/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pico/platform.h"
#include "pico/time.h"
#include "pico/types.h"

#include "hardware/sync.h"

#include "aahrs.h"

#include "../sys/info.h"

#include "flash.h"

// TODO: look into using LittleFS (https://github.com/littlefs-project/littlefs) for a "black box" sort of thing
// (and maybe just replace this flash thing with LittleFS if possible, it's not very good)
// also this exists: https://github.com/lurk101/pico-littlefs

/* Default state of the flash struct (for example, holds default config values).
Refer to the sections' respective enums in flash.h for an indication of what each value is. */
Flash flash = {
    .calibration = { [ 0 ... S_CALIBRATION_HIGHEST] = 0, FLAG_END}, // Holds system-generated calibration values that will be filled in as applicable later
    .pid = {false,
            0.8f, 0.02f, 0.01f, 0.001f, -40, 40, // Roll PID parameters
            1.5f, 0.01f, 0.01f, 0.001f, -20, 20, // Pitch PID parameters
            0.5f, 0.015f, 0.01f, 0.001f, -20, 20, // Yaw PID parameters
            0.1f, 0.01f, 0.05f, 0.001f, -20, -20, // Throttle PID parameters
            FLAG_END},
    .general = {CTRLMODE_2AXIS_ATHR, SWITCH_TYPE_3_POS, 20, 50, 50, true, WIFLY_ENABLED_PASS, false, FLAG_END},
    .control = {0.0025f, 1.5f, 5, // Control handling preferences
                false, 10, 75, 90, 10, 30, 0.015f, // Throttle detent/autothrottle configuration
                180, 0, // Drop bay detent settings
                33, 67, -15, 30, // Control limits
                25, 15, 20, // Physical control surface limits
                20, 0.5f, 1, 1, // Flying wing configuration
                FLAG_END},
    .pins = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 2, 4, // Control IO pins
             16, 17, 21, 20, // Sensor communications pins
             false, false, false, // Servo reverse flags
             FLAG_END},
    .sensors = {IMU_MODEL_BNO055, BARO_MODEL_NONE, GPS_COMMAND_TYPE_PMTK, 9600, FLAG_END},
    .system = {true, false, false, false, false, false, false, 2000, FLAG_END},
    .version = "",
    .wifly_ssid = "pico-fbw",
    .wifly_pass = "wiflyfbw"
};

PrintDefs print;

void flash_erase() {
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
    memcpy(floats, flash.calibration, SIZEOF_FLOAT_SECTORS_BYTES);
    char strings[STRING_SECTOR_SIZE_FULL];
    memcpy(strings, flash.version, SIZEOF_STRING_SECTORS_BYTES);
    // Write data to flash; erase all data first because science, then write
    flash_erase();
    uint32_t offset = GET_PHYSECTOR_LOC(FLOAT_PHYSECTOR);
    uint32_t intr = save_and_disable_interrupts();
    flash_range_program(offset, (uint8_t*)floats, FLASH_SECTOR_SIZE);
    offset = GET_PHYSECTOR_LOC(STRING_PHYSECTOR);
    flash_range_program(offset, (uint8_t*)strings, FLASH_SECTOR_SIZE);
    restore_interrupts(intr);
}

uint flash_load() {
    // This assertion will catch most sizing bugs before they get anywhere (as well as the compiler)--it should ALWAYS be true
    assert((SIZEOF_FLOAT_SECTORS_BYTES + SIZEOF_STRING_SECTORS_BYTES) == sizeof(Flash));
    // These pointers point to the location of our data in the flash chip
    float *floats = (float*)GET_PHYSECTOR_LOC_ABSOLUTE(FLOAT_PHYSECTOR);
    char *strings = (char*)GET_PHYSECTOR_LOC_ABSOLUTE(STRING_PHYSECTOR);
    if (floats[0] != FLAG_BOOT) {
        // Indicates first boot, struct contains default value so write those to flash
        printf("[flash] boot flag not detected, formatting flash now...\n");
        flash.calibration[CALIBRATION_BOOT_FLAG] = FLAG_BOOT;
        flash_save();
    }
    // Flash contains valid data, copy to RAM-based struct
    memcpy(flash.calibration, floats, SIZEOF_FLOAT_SECTORS_BYTES);
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
    return sizeof(Flash);
}
