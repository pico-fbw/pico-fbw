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
// Based off `accel_gyro_temp` example

#define BMI323_ADDR_LOW 0x68
#define BMI323_ADDR_HIGH 0x69

#define BMI323_REG_ID 0x00
#define BMI323_REG_ACC_X 0x03
#define BMI323_REG_GYR_X 0x06
#define BMI323_REG_ACC_CONF 0x20
#define BMI323_REG_GYR_CONF 0x21
#define BMI323_REG_CMD 0x7E

#define BMI323_DEVICE_ID 0x43
#define BMI323_RESET_TRIGGER 0xDEAF
#define BMI323_ACC_RANGE_8G 0x02
#define BMI323_GYR_RANGE_125 0x04
#define BMI323_GYR_RANGE_2000 0x00

// ±8g is default acc range
#define BMI323_RAW_TO_G(x) ((2 << BMI323_ACC_RANGE_8G) / 32768.f) * (x)
// ±2000dps is default gyro range
#define BMI323_RAW_TO_DPS(x) ((125 * (1 << (BMI323_GYR_RANGE_125 - BMI323_GYR_RANGE_2000))) / 32768.f) * (x)

bool bmi323_exists(FusionDriver *self) {
    // Read 4 bytes (2 dummy bytes + ID register)
    byte data[4] = {};
    if (!i2c_read(ASDA, ASCL, BMI323_ADDR_LOW, BMI323_REG_ID, data, sizeof(data))) {
        return false;
    }
    // First (real) byte is the device ID (second is revision ID)
    if (data[2] == BMI323_DEVICE_ID) {
        self->addr = BMI323_ADDR_LOW;
        return true;
    }
    // Main address failed, check the alternate address
    if (!i2c_read(ASDA, ASCL, BMI323_ADDR_HIGH, BMI323_REG_ID, data, sizeof(data))) {
        return false;
    }
    if (data[2] == BMI323_DEVICE_ID) {
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
    // Enable acc and gyro in normal mode
    success &= i2c_write_bits_word(ASDA, ASCL, self->addr, BMI323_REG_ACC_CONF, 0b0111000000000000, 0x04 << 12);
    success &= i2c_write_bits_word(ASDA, ASCL, self->addr, BMI323_REG_GYR_CONF, 0b0111000000000000, 0x04 << 12);
    return success;
}

bool bmi323_read_acc(FusionDriver *self, f32 data[]) {
    byte raw[6];
    if (!i2c_read(ASDA, ASCL, self->addr, BMI323_REG_ACC_X, raw, sizeof(raw))) {
        return false;
    }
    data[0] = BMI323_RAW_TO_G((i16)(raw[1] << 8 | raw[0]));
    data[1] = BMI323_RAW_TO_G((i16)(raw[3] << 8 | raw[2]));
    data[2] = BMI323_RAW_TO_G((i16)(raw[5] << 8 | raw[4]));
    return true;
}

bool bmi323_read_gyro(FusionDriver *self, f32 data[]) {
    byte raw[6];
    if (!i2c_read(ASDA, ASCL, self->addr, BMI323_REG_GYR_X, raw, sizeof(raw))) {
        return false;
    }
    data[0] = BMI323_RAW_TO_DPS((i16)(raw[1] << 8 | raw[0]));
    data[1] = BMI323_RAW_TO_DPS((i16)(raw[3] << 8 | raw[2]));
    data[2] = BMI323_RAW_TO_DPS((i16)(raw[5] << 8 | raw[4]));
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
