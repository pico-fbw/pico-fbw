/**
 * Copyright (c) 2024 Bosch Sensortec GmbH. All rights reserved.
 *
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

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
#define BMI323_GYR_RANGE_500 0x02

// Using ±8g acc range
#define BMI323_RAW_TO_G(x) ((2 << BMI323_ACC_RANGE_8G) / 32768.f) * (x)
// Using ±2000dps gyro range
#define BMI323_RAW_TO_DPS(x) ((125 * (1 << (BMI323_GYR_RANGE_125 - BMI323_GYR_RANGE_500))) / 32768.f) * (x)

// Helper to read the ID register of the BMI323.
static byte read_id_reg(BusConfig *bus, byte reg) {
    // Read 4 bytes (dummy bytes + ID register)
    byte data[4] = {};
    if (!driver_read(bus, reg, data, sizeof(data))) {
        return 0x00;
    }
    // First (non-dummy) byte is the device ID (second is revision ID)
    if (bus->type == BUS_SPI) {
        return data[1]; // Return second byte (SPI has one dummy byte)
    }
    return data[2]; // Return third byte (I2C has two dummy bytes)
}

bool bmi323_exists(FusionDriver *self) {
    return check_devid(&self->bus, BMI323_ADDR_LOW, BMI323_ADDR_HIGH, BMI323_REG_ID, BMI323_DEVICE_ID, read_id_reg);
}

bool bmi323_init(FusionDriver *self) {
    bool success = true;
    // Reset device
    success &= driver_write_word(&self->bus, BMI323_REG_CMD, BMI323_RESET_TRIGGER);
    sleep_ms_blocking(2);
    // Accel: high performance mode, no averaging, filtering to ODR/4, ±8g range, 200hz ODR
    success &= driver_write_word(&self->bus, BMI323_REG_ACC_CONF, 0b0111000010101001);
    // Gyro: high performance mode, no averaging, filtering to ODR/4, ±500dps range, 200hz ODR
    success &= driver_write_word(&self->bus, BMI323_REG_GYR_CONF, 0b0111000010101001);
    return success;
}

bool bmi323_read_acc(FusionDriver *self, f32 data[]) {
    byte raw[8]; // 1-2 dummy bytes (dependant on protocol) + 3 words (6 bytes)
    if (!driver_read(&self->bus, BMI323_REG_ACC_X, raw, sizeof(raw))) {
        return false;
    }
    u8 dBytes = (self->bus.type == BUS_SPI) ? 1 : 2; // 1 dummy byte in SPI, 2 dummy bytes in I2C
    // Convert raw data to signed 16-bit and then to g's
    data[0] = BMI323_RAW_TO_G((i16)(raw[1 + dBytes] << 8 | raw[0 + dBytes]));
    data[1] = BMI323_RAW_TO_G((i16)(raw[3 + dBytes] << 8 | raw[2 + dBytes]));
    data[2] = BMI323_RAW_TO_G((i16)(raw[5 + dBytes] << 8 | raw[4 + dBytes]));
    return true;
}

bool bmi323_read_gyro(FusionDriver *self, f32 data[]) {
    byte raw[8];
    if (!driver_read(&self->bus, BMI323_REG_GYR_X, raw, sizeof(raw))) {
        return false;
    }
    u8 dBytes = (self->bus.type == BUS_SPI) ? 1 : 2;
    data[0] = BMI323_RAW_TO_DPS((i16)(raw[1 + dBytes] << 8 | raw[0 + dBytes]));
    data[1] = BMI323_RAW_TO_DPS((i16)(raw[3 + dBytes] << 8 | raw[2 + dBytes]));
    data[2] = BMI323_RAW_TO_DPS((i16)(raw[5 + dBytes] << 8 | raw[4 + dBytes]));
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
FusionDevice bmi323 = {
    .acc = &bmi323_acc,
    .gyro = &bmi323_gyro,
    .name = "BMI323",
};
