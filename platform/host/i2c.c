/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <string.h>

#include "platform/i2c.h"

bool i2c_setup(u32 sda, u32 scl, u32 freq) {
    return true; // Not implemented
}

bool i2c_read(u32 sda, u32 scl, byte addr, byte reg, byte dest[], size_t len) {
    // Not implemented
    for (u32 i = 0; i < len; i++)
        dest[i] = 0x00;
    return true;
}

bool i2c_write(u32 sda, u32 scl, byte addr, byte reg, const byte src[], size_t len) {
    return true; // Not implemented
}
