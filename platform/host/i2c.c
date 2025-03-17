/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <string.h>

#include "platform/i2c.h"

bool i2c_setup(i16 sda, i16 scl, u32 freq) {
    return true; // Not implemented
    (void)sda;
    (void)scl;
    (void)freq;
}

bool i2c_read(i16 sda, i16 scl, byte addr, byte reg, byte dest[], size_t len) {
    // Not implemented
    for (u32 i = 0; i < len; i++) {
        dest[i] = 0x00;
    }
    return true;
    (void)sda;
    (void)scl;
    (void)addr;
    (void)reg;
    (void)dest;
    (void)len;
}

bool i2c_write(i16 sda, i16 scl, byte addr, byte reg, const byte src[], size_t len) {
    return true; // Not implemented
    (void)sda;
    (void)scl;
    (void)addr;
    (void)reg;
    (void)src;
    (void)len;
}
