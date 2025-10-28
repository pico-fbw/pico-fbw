/**
 * Copyright (c) 2024 Bosch Sensortec GmbH. All rights reserved.
 *
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/i2c.h"
#include "platform/time.h"

#include "drivers.h"

// https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi323-ds000.pdf
// https://github.com/boschsensortec/BMI3XY_SensorAPI

#define BMI323_ADDR_LOW 0x68
#define BMI323_ADDR_HIGH 0x69

#define BMI323_REG_ID 0x00
#define BMI323_REG_ACC_X 0x03
#define BMI323_REG_GYR_X 0x06
#define BMI323_REG_ACC_CONF 0x20
#define BMI323_REG_GYR_CONF 0x21
#define BMI323_REG_CMD 0x7E

#define BMI323_DEVICE_ID 0x43
#define BMI323_REVISION_ID_MASK 0xF0
#define BMI323_REVISION_ID_POS 4
#define BMI323_RESET_TRIGGER 0xDEAF

bool bmi323_exists(FusionDriver *self) {
    byte data[4] = {};
    if (!i2c_read(ASDA, ASCL, BMI323_ADDR_LOW, BMI323_REG_ID, data, sizeof(data))) {
        return false;
    }
    if (data[3] == BMI323_DEVICE_ID) {
        self->addr = BMI323_ADDR_LOW;
        return true;
    }
    // Main address failed, check the alternate address
    if (!i2c_read(ASDA, ASCL, BMI323_ADDR_HIGH, BMI323_REG_ID, data, sizeof(data))) {
        return false;
    }
    if (data[3] == BMI323_DEVICE_ID) {
        self->addr = BMI323_ADDR_HIGH;
        return true;
    }
    return false;
}

bool bmi323_init(FusionDriver *self) {
    bool success = true;
    // Reset device
    success &= i2c_write_word(ASDA, ASCL, self->addr, BMI323_REG_CMD, BMI323_RESET_TRIGGER);
    sleep_ms_blocking(2);
    // Unfinished
    return success;
}

bool bmi323_read_acc(FusionDriver *self, f32 data[]) {
    // Unfinished
    return true;
}

bool bmi323_read_gyro(FusionDriver *self, f32 data[]) {
    // Unfinished
    return true;
}

FusionDriver bmi323_acc = {
    .exists = bmi323_exists,
    .init = bmi323_init,
    .read = bmi323_read_acc,
};
FusionDriver bmi323_gyro = {
    .exists = bmi323_exists,
    .init = bmi323_init,
    .read = bmi323_read_gyro,
};
const FusionDevice bmi323 = {
    .acc = &bmi323_acc,
    .gyro = &bmi323_gyro,
    .name = "BMI323",
};
