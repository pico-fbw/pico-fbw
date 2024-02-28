/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <string.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "platform/i2c.h"

#define I2C_TIMEOUT_US 100E3 // The timeout for i2c operations in microseconds

/**
 * @param sda the pin number of the SDA pin
 * @param scl the pin number of the SCL pin
 * @return the i2c instance that the pins lie on, or NULL if the pins do not form a valid i2c instance
 */
static inline i2c_inst_t *i2c_inst_from_pins(u32 sda, u32 scl) {
    switch (sda) {
    case 0:
    case 4:
    case 8:
    case 12:
    case 16:
    case 20:
        switch (scl) {
        case 1:
        case 5:
        case 9:
        case 13:
        case 17:
        case 21:
            return i2c0;
        default:
            return NULL;
        }
        break;
    case 2:
    case 6:
    case 10:
    case 14:
    case 18:
    case 26:
        switch (scl) {
        case 3:
        case 7:
        case 11:
        case 15:
        case 19:
        case 27:
            return i2c1;
        default:
            return NULL;
        }
        break;
    default:
        return NULL;
    }
}

bool i2c_setup(u32 sda, u32 scl, u32 freq) {
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
    // Find the i2c instance (0 or 1) that the pins lie on
    i2c_inst_t *i2c = i2c_inst_from_pins(sda, scl);
    if (i2c == NULL)
        return false;
    i2c_init(i2c, freq);
    return true;
}

bool i2c_read(u32 sda, u32 scl, byte addr, byte reg, byte dest[], size_t len) {
    i2c_inst_t *i2c = i2c_inst_from_pins(sda, scl);
    if (i2c == NULL)
        return false;
    // First, write the register that we want to read from
    i32 timeout = i2c_write_timeout_us(i2c, addr, &reg, sizeof(reg), true, I2C_TIMEOUT_US);
    // The above function returns the number of bytes written, so we can use it to check if the write was successful
    if (timeout != sizeof(reg))
        return false;
    // Now, read the data from the register into the destination buffer
    timeout = i2c_read_timeout_us(i2c, addr, dest, len, false, I2C_TIMEOUT_US);
    return timeout == (i32)len;
}

bool i2c_write(u32 sda, u32 scl, byte addr, byte reg, byte src[], size_t len) {
    i2c_inst_t *i2c = i2c_inst_from_pins(sda, scl);
    if (i2c == NULL)
        return false;
    // Create and write a command buffer that contains the register first, followed by the data to write
    byte cmd[len + 1];
    cmd[0] = reg;
    memcpy(cmd + 1, src, len);
    i32 timeout = i2c_write_timeout_us(i2c, addr, cmd, sizeof(cmd), false, I2C_TIMEOUT_US);
    return timeout == (i32)sizeof(cmd);
}
