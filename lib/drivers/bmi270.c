/**
 * Copyright (c) 2023 Bosch Sensortec GmbH. All rights reserved.
 *
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/i2c.h"
#include "platform/time.h"

#include "firmware/bmi270.bin.h"

#include "drivers.h"

// https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi270-ds000.pdf
// https://github.com/boschsensortec/BMI270_SensorAPI

#define BMI270_ADDR_LOW 0x68
#define BMI270_ADDR_HIGH 0x69

#define BMI270_REG_ID 0x00
#define BMI270_REG_ACC_X 0x0D
#define BMI270_REG_GYR_X 0x12
#define BMI270_REG_INTERNAL_STATUS 0x21
#define BMI270_REG_GYR_CONF 0x42
#define BMI270_REG_INIT_CTRL 0x59
#define BMI270_REG_INIT_DATA 0x5E
#define BMI270_REG_PWR_CONF 0x7C
#define BMI270_REG_PWR_CTRL 0x7D
#define BMI270_REG_CMD 0x7E

#define BMI270_DEVICE_ID 0x24
#define BMI270_RESET_TRIGGER 0xB6
#define BMI270_ACC_RANGE_8G 0x02
#define BMI270_GYR_RANGE_125 0x04
#define BMI270_GYR_RANGE_2000 0x00

// ±8g is default acc range
#define BMI270_RAW_TO_G(x) ((2 << BMI270_ACC_RANGE_8G) / 32768.f) * (x)
// ±2000dps is default gyro range
#define BMI270_RAW_TO_DPS(x) ((125 * (1 << (BMI270_GYR_RANGE_125 - BMI270_GYR_RANGE_2000))) / 32768.f) * (x)

bool bmi270_exists(FusionDriver *self) {
    return check_devid(self, BMI270_ADDR_LOW, BMI270_ADDR_HIGH, BMI270_REG_ID, BMI270_DEVICE_ID);
}

bool bmi270_init(FusionDriver *self) {
    bool success = true;
    // Reset device
    success &= i2c_write_byte(ASDA, ASCL, self->addr, BMI270_REG_CMD, BMI270_RESET_TRIGGER);
    sleep_ms_blocking(2);
    // Disable advanced power save
    success &= i2c_write_bits(ASDA, ASCL, self->addr, BMI270_REG_PWR_CONF, 0b00000001, 0);
    sleep_us_blocking(450);
    // Write config data
    success &= i2c_write_byte(ASDA, ASCL, self->addr, BMI270_REG_INIT_CTRL, 0x00);
    // FIXME: properly upload config data in chunks
    success &= i2c_write(ASDA, ASCL, self->addr, BMI270_REG_INIT_DATA, bmi270_bin, sizeof(bmi270_bin));
    success &= i2c_write_byte(ASDA, ASCL, self->addr, BMI270_REG_INIT_CTRL, 0x01);
    // Wait up to 20ms for device to signal that it is ready
    Timestamp timeout = timestamp_in_ms(20);
    bool ready = false;
    while (!ready || timestamp_reached(&timeout)) {
        ready = i2c_read_bits(ASDA, ASCL, self->addr, BMI270_REG_INTERNAL_STATUS, 0b00001111) == 0b0001;
    }
    if (!ready || !success) {
        return false;
    }
    // Enable accelerometer and gyroscope (w/ noise performance on), disable temperature and auxillary sensors
    success &= i2c_write_bits(ASDA, ASCL, self->addr, BMI270_REG_PWR_CTRL, 0b00001111, 0b0110);
    success &= i2c_write_bits(ASDA, ASCL, self->addr, BMI270_REG_GYR_CONF, 0b01000000, 1);
    return success;
}

bool bmi270_read_acc(FusionDriver *self, f32 data[]) {
    byte raw[6];
    if (!i2c_read(ASDA, ASCL, self->addr, BMI270_REG_ACC_X, raw, sizeof(raw))) {
        return false;
    }
    data[0] = BMI270_RAW_TO_G((i16)(raw[1] << 8 | raw[0]));
    data[1] = BMI270_RAW_TO_G((i16)(raw[3] << 8 | raw[2]));
    data[2] = BMI270_RAW_TO_G((i16)(raw[5] << 8 | raw[4]));
    return true;
}

bool bmi270_read_gyro(FusionDriver *self, f32 data[]) {
    byte raw[6];
    if (!i2c_read(ASDA, ASCL, self->addr, BMI270_REG_GYR_X, raw, sizeof(raw))) {
        return false;
    }
    data[0] = BMI270_RAW_TO_DPS((i16)(raw[1] << 8 | raw[0]));
    data[1] = BMI270_RAW_TO_DPS((i16)(raw[3] << 8 | raw[2]));
    data[2] = BMI270_RAW_TO_DPS((i16)(raw[5] << 8 | raw[4]));
    return true;
}

FusionDriver bmi270_acc = {
    .exists = bmi270_exists,
    .init = bmi270_init,
    .read = bmi270_read_acc,
};
FusionDriver bmi270_gyro = {
    .exists = bmi270_exists,
    .init = bmi270_init,
    .read = bmi270_read_gyro,
};
const FusionDevice bmi270 = {
    .acc = &bmi270_acc,
    .gyro = &bmi270_gyro,
    .name = "BMI270",
};
