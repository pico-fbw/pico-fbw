/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "platform/defs.h"

#include "platform/i2c.h"

typedef struct I2CInstance {
    const char *device;
    int fd;
} I2CInstance;

static I2CInstance instances[MAX_I2C_DEVICES];

/**
 * @return a pointer to the I2C instance that uses the given pins, or NULL if no such instance exists
 */
static I2CInstance *i2c_instance_from_pins(i16 sda, i16 scl) {
    for (u32 i = 0; i < MAX_I2C_DEVICES; i++) {
        I2CMapping mapping = I2C_MAP[i];
        if (mapping.sda == sda && mapping.scl == scl) {
            instances[i].device = mapping.device;
            return &instances[i];
        }
    }
    return NULL;
}

bool i2c_setup(i16 sda, i16 scl, u32 freq) {
    I2CInstance *inst = i2c_instance_from_pins(sda, scl);
    if (inst == NULL) {
        return false;
    }
    inst->fd = open(inst->device, O_RDWR);
    if (inst->fd < 0) {
        return false;
    }
    return true;
    (void)freq; // Frequency is managed by the kernel driver
}

bool i2c_read(i16 sda, i16 scl, byte addr, byte reg, byte dest[], size_t len) {
    I2CInstance *inst = i2c_instance_from_pins(sda, scl);
    if (inst == NULL || inst->fd < 0) {
        return false;
    }
    // Set slave address for this transaction
    if (ioctl(inst->fd, I2C_SLAVE, addr) < 0) {
        return false;
    }
    // Write the register address to read from
    if (write(inst->fd, &reg, 1) != 1) {
        return false;
    }
    // Read the data
    return read(inst->fd, dest, len) == (ssize_t)len;
}

bool i2c_write(i16 sda, i16 scl, byte addr, byte reg, const byte src[], size_t len) {
    I2CInstance *inst = i2c_instance_from_pins(sda, scl);
    if (inst == NULL || inst->fd < 0) {
        return false;
    }
    if (ioctl(inst->fd, I2C_SLAVE, addr) < 0) {
        return false;
    }
    byte cmd[len + 1];
    cmd[0] = reg;
    memcpy(cmd + 1, src, len);
    return write(inst->fd, cmd, len + 1) == (ssize_t)(len + 1);
}
