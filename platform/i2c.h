#pragma once

#include <stdbool.h>
#include "platform/types.h"

/**
 * Sets up the given SDA and SCL pins for I2C communication at the given frequency.
 * @param sda the SDA pin to use
 * @param scl the SCL pin to use
 * @param freq the frequency to run the I2C bus at, in Hz
 * @return true if the setup was successful
 * @note Many platforms have limitations on which pins and frequencies can be used for I2C.
 * This means that the frequency that you request may not be the exact frequency that is set.
 */
bool i2c_setup(u32 sda, u32 scl, u32 freq);

/**
 * Reads `len` bytes from `addr` at `reg` and stores them in `dest[]`.
 * @param sda the SDA pin to use
 * @param scl the SCL pin to use
 * @param addr the I2C address to read from
 * @param reg the register to read from
 * @param dest the buffer to read the data into
 * @param len the number of bytes to read
 * @return true if the read was successful
 * @note `dest[]` must be large enough to hold `len` bytes of data.
 * @note `sda` and `scl` must be set up with `i2c_setup()` before calling this function.
 */
bool i2c_read(u32 sda, u32 scl, byte addr, byte reg, byte dest[], size_t len);

/**
 * Writes `len` bytes from `src[]` to `addr` at `reg`.
 * @param sda the SDA pin to use
 * @param scl the SCL pin to use
 * @param addr the I2C address to write to
 * @param reg the register to write to
 * @param src the buffer of data to write
 * @param len the number of bytes to write
 * @return true if the write was successful
 * @note `src[]` must contain at least `len` bytes of data
 * @note `sda` and `scl` must be set up with `i2c_setup()` before calling this function.
 */
bool i2c_write(u32 sda, u32 scl, byte addr, byte reg, const byte src[], size_t len);
