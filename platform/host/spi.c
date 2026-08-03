/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/spi.h"

bool spi_setup(i16 clk, i16 mosi, i16 miso, u32 freq) {
    return true; // Not implemented
    (void)clk;
    (void)mosi;
    (void)miso;
    (void)freq;
}

bool spi_transfer(i16 clk, i16 mosi, i16 miso, i16 cs, const byte tx[], byte rx[], size_t len) {
    memset(rx, 0, len);
    return true; // Not implemented
    (void)clk;
    (void)mosi;
    (void)miso;
    (void)cs;
    (void)tx;
}
