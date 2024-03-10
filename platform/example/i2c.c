/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/i2c.h"

bool i2c_setup(u32 sda, u32 scl, u32 freq) {
    // This function will be called before executing any other I2C-related functions on a given SDA/SCL pair.
    // It should configure the given SDA and SCL pins for I2C communication at the given frequency (in Hz).
    // It should return true if the setup was successful, false if not.
}

bool i2c_read(u32 sda, u32 scl, byte addr, byte reg, byte dest[], size_t len) {
    // This function should read `len` bytes from an 8-bit I2C device address `addr` at an 8-bit I2C register `reg` and store
    // them in the array `dest[]`. For example, if `addr` is 0x69, `reg` is 0x01, and `len` is 1, this function should initiate
    // an I2C transaction to read 1 byte from register 0x01 of device 0x69 and store it in `dest[0]`. Note that many I2C devices
    // require a write transaction to set the register to read from before reading, so you may need to write to the device
    // before reading from it. This function should return true if the read was successful, false if not.
}

bool i2c_write(u32 sda, u32 scl, byte addr, byte reg, byte src[], size_t len) {
    // This function should write `len` bytes from the array `src[]` to an 8-bit I2C device address `addr` at an 8-bit I2C
    // register `reg`. For example, if `addr` is 0x69, `reg` is 0x01, and `len` is 1, this function should initiate an I2C
    // transaction to write 1 byte to register 0x01 of device 0x69 with the value `src[0]`. This function should return true if
    // the write was successful, false if not.
}
