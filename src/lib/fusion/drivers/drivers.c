/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * Copyright (c) 2020 Bjarne Hansen
 * All rights reserved.
 *
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
*/

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include <string.h>

#include "pico/types.h"

#include "hardware/gpio.h"

#include "../../../io/flash.h"

#include "../fconfig.h"

#include "drivers.h"

/**
 * Reads a byte to from addr at reg and stores it in dest.
 * @param addr The I2C address of the device
 * @param reg The register to read from
 * @param dest The destination buffer
 * @return True if the read was successful
*/
static bool i2cReadByte(unsigned char addr, unsigned char reg, unsigned char *dest) {
    int timeout = i2c_write_timeout_us(DRIVER_I2C, addr, &reg, sizeof(reg), true, DRIVER_TIMEOUT_US);
    if (timeout != sizeof(reg)) return false;
    timeout = i2c_read_timeout_us(DRIVER_I2C, addr, dest, sizeof(*dest), false, DRIVER_TIMEOUT_US);
    return timeout == sizeof(*dest);
}

/**
 * Reads len bytes from addr at reg and stores them in dest.
 * @param addr The I2C address of the device
 * @param reg The register to read from
 * @param dest The destination buffer
 * @param len The number of bytes to read
 * @return True if the read was successful
*/
static bool i2cReadBytes(unsigned char addr, unsigned char reg, unsigned char *dest, size_t len) {
    int timeout = i2c_write_timeout_us(DRIVER_I2C, addr, &reg, sizeof(reg), true, DRIVER_TIMEOUT_US);
    if (timeout != sizeof(reg)) return false;
    timeout = i2c_read_timeout_us(DRIVER_I2C, addr, dest, len, false, DRIVER_TIMEOUT_US);
    return timeout == len;
}

/**
 * Writes val to addr at reg.
 * @param addr The I2C address of the device
 * @param reg The register to write to
 * @param val The value to write
 * @return True if the write was successful
*/
static bool i2cWriteByte(unsigned char addr, unsigned char reg, unsigned char val) {
    unsigned char c[] = {reg, val};
    int timeout = i2c_write_timeout_us(DRIVER_I2C, addr, c, sizeof(c), true, DRIVER_TIMEOUT_US);
    return timeout == sizeof(c);
}

/**
 * Writes val[len] to addr at reg.
 * @param addr The I2C address of the device
 * @param reg The register to write to
 * @param val The values to write
 * @param len The length of the value
 * @return True if the write was successful
*/
static bool i2cWriteBytes(unsigned char addr, unsigned char reg, const unsigned char *val, size_t len) {
    unsigned char c[len + 1];
    c[0] = reg;
    memcpy(&c[1], val, len);
    int timeout = i2c_write_timeout_us(DRIVER_I2C, addr, c, len + 1, true, DRIVER_TIMEOUT_US);
    return timeout == (len + 1);
}

void driver_init() {
    if (print.fbw) printf("[AAHRS] initializing ");
    if (DRIVER_I2C == i2c0) {
        if (print.fbw) printf("i2c0 ");
    } else if (DRIVER_I2C == i2c1) {
        if (print.fbw) printf("i2c1 ");
    }
    if (print.fbw) printf("at %d kHz, on pins %d (SDA) and %d (SCL)\n", I2C_FREQ_KHZ,
                          (uint)flash.pins[PINS_AAHRS_SDA], (uint)flash.pins[PINS_AAHRS_SCL]);
    gpio_set_function((uint)flash.pins[PINS_AAHRS_SDA], GPIO_FUNC_I2C);
    gpio_set_function((uint)flash.pins[PINS_AAHRS_SCL], GPIO_FUNC_I2C);
    gpio_pull_up((uint)flash.pins[PINS_AAHRS_SDA]);
    gpio_pull_up((uint)flash.pins[PINS_AAHRS_SCL]);
    i2c_init(DRIVER_I2C, I2C_FREQ_KHZ * 1000);
}

int32_t driver_read(registerDeviceInfo_t *devInfo, uint16_t peripheralAddress, const registerReadlist_t *pReadList, uint8_t *pOutBuf) {
    uint8_t *pBuf;

    // Validate handle
    if (pReadList == NULL || pOutBuf == NULL) {
        return SENSOR_ERROR_BAD_ADDRESS;
    }
    const registerReadlist_t *pCmd = pReadList;

    // Traverse the read list and read the registers one by one unless the register read list numBytes is zero
    for (pBuf = pOutBuf; pCmd->numBytes != 0; pCmd++) {
        if (!i2cReadBytes((unsigned char)peripheralAddress, (unsigned char)pCmd->readFrom, pBuf, pCmd->numBytes)) {
            return SENSOR_ERROR_READ;
        }
        pBuf += pCmd->numBytes;
    }
    return SENSOR_ERROR_NONE;
}

int32_t driver_read_register(registerDeviceInfo_t *devInfo, uint16_t peripheralAddress, uint8_t offset, uint8_t len, uint8_t *pOutBuf) {
    if (i2cReadBytes((unsigned char)peripheralAddress, offset, pOutBuf, len)) {
        return SENSOR_ERROR_NONE;
    } else {
        return SENSOR_ERROR_READ;
    }
}

int8_t driver_write_list(registerDeviceInfo_t *devInfo, uint16_t peripheralAddress, const registerwritelist_t *pRegWriteList) {
    // Validate handle
    if (pRegWriteList == NULL) {
        return SENSOR_ERROR_BAD_ADDRESS;
    }

    const registerwritelist_t *pCmd = pRegWriteList;
    // Update register values based on register write list until the next cmd is the list terminator.
    while (pCmd->writeTo != 0xFFFF) {
        // Set the register based on the values in the register value pair
        if (!i2cWriteByte((unsigned char)peripheralAddress, (unsigned char)pCmd->writeTo, pCmd->value)) {
            return SENSOR_ERROR_WRITE;
        }
        ++pCmd;
    }
    return SENSOR_ERROR_NONE;
}
