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
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/i2c.h"

#include "lib/fusion/fconfig.h"

#include "sys/configuration.h"
#include "sys/print.h"

#include "drivers.h"

/**
 * Reads `len` bytes from `addr` at `reg` and stores them in `dest`.
 * @param addr the I2C address of the device
 * @param reg the register to read from
 * @param dest the destination buffer
 * @param len the number of bytes to read
 * @return true if the read was successful
 */
static bool i2cReadBytes(byte addr, byte reg, byte *dest, size_t len) {
    return i2c_read((u32)config.pins[PINS_AAHRS_SDA], (u32)config.pins[PINS_AAHRS_SCL], addr, reg, dest, len);
}

/**
 * Writes `val` to `addr` at `reg`.
 * @param addr the I2C address of the device
 * @param reg the register to write to
 * @param val the value to write
 * @return true if the write was successful
 */
static bool i2cWriteByte(byte addr, byte reg, byte val) {
    byte buf[] = {val};
    return i2c_write((u32)config.pins[PINS_AAHRS_SDA], (u32)config.pins[PINS_AAHRS_SCL], addr, reg, buf, 1);
}

void driver_init() {
    printfbw(aahrs, "initializing i2c0 at %d kHz, on pins %d (SDA) and %d (SCL)", DRIVER_FREQ_KHZ,
             (u32)config.pins[PINS_AAHRS_SDA], (u32)config.pins[PINS_AAHRS_SCL]);
    i2c_setup((u32)config.pins[PINS_AAHRS_SDA], (u32)config.pins[PINS_AAHRS_SCL], DRIVER_FREQ_KHZ * 1000);
}

i32 driver_read(registerDeviceInfo_t *devInfo, u16 peripheralAddress, const registerReadList_t *pReadList, byte *pOutBuf) {
    byte *pBuf;

    // Validate handle
    if (!pReadList || !pOutBuf) {
        return SENSOR_ERROR_BAD_ADDRESS;
    }
    const registerReadList_t *pCmd = pReadList;

    // Traverse the read list and read the registers one by one unless the register read list numBytes is zero
    for (pBuf = pOutBuf; pCmd->numBytes != 0; pCmd++) {
        if (!i2cReadBytes((byte)peripheralAddress, (byte)pCmd->readFrom, pBuf, pCmd->numBytes)) {
            return SENSOR_ERROR_READ;
        }
        pBuf += pCmd->numBytes;
    }
    return SENSOR_ERROR_NONE;
}

i32 driver_read_register(registerDeviceInfo_t *devInfo, u16 peripheralAddress, byte offset, byte len, byte *pOutBuf) {
    if (i2cReadBytes((byte)peripheralAddress, offset, pOutBuf, len)) {
        return SENSOR_ERROR_NONE;
    } else {
        return SENSOR_ERROR_READ;
    }
}

i8 driver_write_list(registerDeviceInfo_t *devInfo, u16 peripheralAddress, const registerWriteList_t *pRegWriteList) {
    // Validate handle
    if (!pRegWriteList) {
        return SENSOR_ERROR_BAD_ADDRESS;
    }

    const registerWriteList_t *pCmd = pRegWriteList;
    // Update register values based on register write list until the next cmd is the list terminator.
    while (pCmd->writeTo != 0xFFFF) {
        // Set the register based on the values in the register value pair
        if (!i2cWriteByte((byte)peripheralAddress, (byte)pCmd->writeTo, pCmd->value)) {
            return SENSOR_ERROR_WRITE;
        }
        ++pCmd;
    }
    return SENSOR_ERROR_NONE;
}
