#pragma once

#include "platform/types.h"

/**
 * Sets up the given CLK, MOSI, and MISO pins for SPI communication at the given frequency.
 * @param clk the CLK pin to use
 * @param mosi the MOSI pin to use
 * @param miso the MISO pin to use
 * @param freq the frequency to run the bus at, in Hz
 * @return true if the setup was successful
 * @note Many platforms have limitations on which pins and frequencies can be used for SPI.
 * Therefore, the frequency that you request may not be the exact frequency that is set.
 */
bool spi_setup(i16 clk, i16 mosi, i16 miso, u32 freq);

/**
 * Transmits and receives `len` bytes from `tx[]` and into `rx[]`, asserting `cs` during the transaction.
 * @param clk the CLK pin to use
 * @param mosi the MOSI pin to use
 * @param miso the MISO pin to use
 * @param cs the CS pin to assert
 * @param tx buffer containing data to transmit
 * @param rx buffer containing data to receive
 * @param len the number of bytes to read
 * @return true if the transaction was successful
 * @note `tx[]` and `rx[]` must be large enough to hold `len` bytes of data.
 * @note `clk`, `mosi`, and `miso` must be set up with `spi_setup()` before calling this function.
 */
bool spi_transfer(i16 clk, i16 mosi, i16 miso, i16 cs, const byte tx[], byte rx[], size_t len);
