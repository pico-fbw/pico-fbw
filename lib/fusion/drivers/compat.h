/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

/**
 * Compatability functions
 * Since the IMU system was originally written for mongoose-os, these functions provite compatibility with the
 * original mongoose-os system calls. This means it is compatible with all drivers already written with minimal porting effort.
 * If you are writing a new driver, it doesn't really matter whether you use mongoose-os's functions or pico-fbw's,
 * they both behave identically at their core. It's whichever you prefer.
 */

#pragma once

#include "platform/i2c.h"
#include "platform/int.h"
#include "platform/time.h"

#include "sys/configuration.h"

#define SDA (u32)(config.pins[PINS_AAHRS_SDA])
#define SCL (u32)(config.pins[PINS_AAHRS_SCL])

/*
 * Helper for reading 1-byte register `reg` from a device at address `addr`.
 * In case of success return a numeric byte value from 0x00 to 0xff; otherwise return -1.
 */
static inline i32 mgos_i2c_read_reg_b(u16 addr, byte reg) {
    byte read;
    if (!i2c_read(SDA, SCL, (byte)addr, reg, &read, 1))
        return -1;
    return read;
}

/*
 * Helper for reading `n`-byte register value from a device.
 * Returns true on success, false on error.
 * Data is written to `buf`, which should be large enough.
 */
static inline bool mgos_i2c_read_reg_n(u16 addr, byte reg, size_t n, u8 *buf) {
    return i2c_read(SDA, SCL, (byte)addr, reg, buf, n);
}

/*
 * Helper for writing 1-byte register `reg` to a device at address `addr`.
 * Returns `true` in case of success, `false` otherwise.
 */
static inline bool mgos_i2c_write_reg_b(u16 addr, byte reg, u8 value) {
    return i2c_write(SDA, SCL, (byte)addr, reg, (byte[]){value}, 1);
}

/*
 * Helper for writing `n`-byte register `reg` to a device at address `addr`.
 * Returns `true` in case of success, `false` otherwise.
 */
static inline bool mgos_i2c_write_reg_n(u16 addr, byte reg, size_t n, const u8 *buf) {
    return i2c_write(SDA, SCL, (byte)addr, reg, buf, n);
}

/*
 * Helper to set/get a number of bits in a register `reg` on a device at
 * address `addr`.
 * - bitoffset: 0..n, is the position at which to write `value`
 *              n=7 for reg_b, n=15 for reg_w
 * - bitlen   : 1..m, number of bits to write
 *              m=8 for reg_b, m=16 for reg_w
 * - value    : the value to write there
 *
 * Invariants:
 * - value must fit in `bitlen` (ie value < 2^bitlen)
 * - bitlen+bitoffset <= register size (8 for reg_b, 16 for reg_w)
 * - bitlen cannot be 0.
 * - *conn cannot be NULL.
 *
 * The `setbits` call will write the bits to the register, the `getbits` call
 * will return the value of those bits in *value.
 *
 * Returns `true` in case of success, `false` otherwise.
 *
 * Examples (the bits set or get are between []):
 * 1) If register was 0x00 (0b00000000)
 *    mgos_i2c_setbits_reg_b(conn, addr, reg, 0, 1, 1);
 *    Register will be 0x01 (0b0000000[1])
 * 2) If register was 0xffff (0b1111111111111111)
 *    mgos_i2c_setbits_reg_w(conn, addr, reg, 14, 2, 0);
 *    Register will be 0x3fff (0b[00]11111111111111)
 * 3) If register was 0xabcd (0b1010101111001101)
 *    mgos_i2c_setbits_reg_w(conn, addr, reg, 10, 4, 5);
 *    Register will be 0x97cd (0b10[0101]1111001101)
 * 4) If register was 0xabcd (0b1010101111001101)
 *    mgos_i2c_getbits_reg_w(conn, addr, reg, 0, 2, &value);
 *    value will be 1   (0b10101011110011[01])
 *    mgos_i2c_getbits_reg_w(conn, addr, reg, 13, 3, &value);
 *    value will be 5   (0b[101]0101111001101)
 *    mgos_i2c_getbits_reg_w(conn, addr, reg, 6, 5, &value);
 *    value will be 15  (0b10101[01111]001101)
 *    mgos_i2c_getbits_reg_w(conn, addr, reg, 5, 9, &value);
 *    value will be 350 (0b10[101011110]01101)
 */
static inline bool mgos_i2c_setbits_reg_b(u16 addr, byte reg, u8 bitoffset, u8 bitlen, u8 value) {
    u8 old, new;

    if (bitoffset + bitlen > 8 || bitlen == 0)
        return false;
    if (value > (1 << bitlen) - 1)
        return false;

    if (!i2c_read(SDA, SCL, addr, reg, &old, 1))
        return false;

    new = old | (((1 << bitlen) - 1) << bitoffset);
    new &= ~(((1 << bitlen) - 1) << bitoffset);
    new |= (value) << bitoffset;

    return i2c_write(SDA, SCL, addr, reg, &new, 1);
}
static inline bool mgos_i2c_getbits_reg_b(u16 addr, byte reg, u8 bitoffset, u8 bitlen, u8 *value) {
    u8 val, mask;

    if (bitoffset + bitlen > 8 || bitlen == 0 || !value)
        return false;

    if (!i2c_read(SDA, SCL, addr, reg, &val, 1))
        return false;

    mask = ((1 << bitlen) - 1);
    mask <<= bitoffset;
    val &= mask;
    val >>= bitoffset;

    *value = val;
    return true;
}

/* Delay given number of microseconds */
static inline void mgos_usleep(u32 usec) {
    sleep_us_blocking((u64)usec);
}