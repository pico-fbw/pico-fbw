#pragma once

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
bool i2c_setup(i16 sda, i16 scl, u32 freq);

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
bool i2c_read(i16 sda, i16 scl, byte addr, byte reg, byte dest[], size_t len);

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
bool i2c_write(i16 sda, i16 scl, byte addr, byte reg, const byte src[], size_t len);

/**
 * Reads a single byte from `addr` at `reg`.
 * @param sda the SDA pin to use
 * @param scl the SCL pin to use
 * @param addr the I2C address to read from
 * @param reg the register to read from
 * @return the byte read
 * @note `sda` and `scl` must be set up with `i2c_setup()` before calling this function.
 */
static inline byte i2c_read_byte(i16 sda, i16 scl, byte addr, byte reg) {
    byte data = 0x00;
    i2c_read(sda, scl, addr, reg, &data, 1);
    return data;
}

/**
 * Writes a single byte from `data` to `addr` at `reg`.
 * @param sda the SDA pin to use
 * @param scl the SCL pin to use
 * @param addr the I2C address to write to
 * @param reg the register to write to
 * @param data the byte to write
 * @return true if the write was successful
 * @note `sda` and `scl` must be set up with `i2c_setup()` before calling this function.
 */
static inline bool i2c_write_byte(i16 sda, i16 scl, byte addr, byte reg, byte data) {
    return i2c_write(sda, scl, addr, reg, (byte[]){data}, 1);
}

/**
 * Reads a 16-bit word from `addr` at `reg`.
 * @param sda the SDA pin to use
 * @param scl the SCL pin to use
 * @param addr the I2C address to read from
 * @param reg the register to read from
 * @return the word read
 * @note `sda` and `scl` must be set up with `i2c_setup()` before calling this function.
 */
static inline word i2c_read_word(i16 sda, i16 scl, byte addr, byte reg) {
    byte raw[2] = {};
    i2c_read(sda, scl, addr, reg, raw, sizeof(raw));
    return (word)(raw[1] << 8 | raw[0]);
}

/**
 * Writes a 16-bit word `data` to `addr` at `reg`.
 * @param sda the SDA pin to use
 * @param scl the SCL pin to use
 * @param addr the I2C address to write to
 * @param reg the register to write to
 * @param data the word to write
 * @return true if the write was successful
 * @note `sda` and `scl` must be set up with `i2c_setup()` before calling this function.
 */
static inline bool i2c_write_word(i16 sda, i16 scl, byte addr, byte reg, word data) {
    return i2c_write(sda, scl, addr, reg, (byte[]){data & 0xFF, (data >> 8) & 0xFF}, 2);
}

/**
 * Reads the bits specified by `mask` from `addr` at `reg`.
 * @param sda the SDA pin to use
 * @param scl the SCL pin to use
 * @param addr the I2C address to read from
 * @param reg the register to read from
 * @param mask the bits to read
 * @return the bits read
 * @note `sda` and `scl` must be set up with `i2c_setup()` before calling this function.
 */
static inline byte i2c_read_bits(i16 sda, i16 scl, byte addr, byte reg, byte mask) {
    byte value = i2c_read_byte(sda, scl, addr, reg);
    return value & mask;
}

/**
 * Writes `data` to `addr` at `reg` with the bits specified by `mask` set to `data`.
 * @param sda the SDA pin to use
 * @param scl the SCL pin to use
 * @param addr the I2C address to write to
 * @param reg the register to write to
 * @param mask the bits to set
 * @param data the data to write
 * @return true if the write was successful
 * @note `sda` and `scl` must be set up with `i2c_setup()` before calling this function.
 */
static inline bool i2c_write_bits(i16 sda, i16 scl, byte addr, byte reg, byte mask, byte data) {
    byte value = i2c_read_byte(sda, scl, addr, reg);
    value &= ~mask;
    value |= data & mask;
    return i2c_write_byte(sda, scl, addr, reg, value);
}

/**
 * Reads the bits specified by `mask` from `addr` at `reg`.
 * @param sda the SDA pin to use
 * @param scl the SCL pin to use
 * @param addr the I2C address to read from
 * @param reg the register to read from
 * @param mask the bits to read
 * @return the bits read
 * @note `sda` and `scl` must be set up with `i2c_setup()` before calling this function.
 */
static inline word i2c_read_bits_word(i16 sda, i16 scl, byte addr, byte reg, word mask) {
    word value = i2c_read_word(sda, scl, addr, reg);
    return value & mask;
}

/**
 * Writes `data` to `addr` at `reg` with the bits specified by `mask` set to `data`.
 * @param sda the SDA pin to use
 * @param scl the SCL pin to use
 * @param addr the I2C address to write to
 * @param reg the register to write to
 * @param mask the bits to set
 * @param data the data to write
 * @return true if the write was successful
 * @note `sda` and `scl` must be set up with `i2c_setup()` before calling this function.
 */
static inline bool i2c_write_bits_word(i16 sda, i16 scl, byte addr, byte reg, word mask, word data) {
    word value = i2c_read_word(sda, scl, addr, reg);
    value &= ~mask;
    value |= data & mask;
    return i2c_write_word(sda, scl, addr, reg, value);
}
