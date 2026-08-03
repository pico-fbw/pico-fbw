/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/spi.h"

bool spi_setup(i16 clk, i16 mosi, i16 miso, u32 freq) {
    // This function will be called before executing any other SPI-related functions on a given CLK/MOSI/MISO set.
    // It should configure the given pins for SPI communication at the given frequency (in Hz).
    // It should return true if the setup was successful, false if not.
}

bool spi_transfer(i16 clk, i16 mosi, i16 miso, i16 cs, const byte tx[], byte rx[], size_t len) {
    // This function should perform a full-duplex SPI transaction, transmitting the `tx[]` buffer while receiving into
    // the `rx[]` buffer. This should be done for `len` bytes. This function should return true if the transaction was
    // successful, false if not.
}
