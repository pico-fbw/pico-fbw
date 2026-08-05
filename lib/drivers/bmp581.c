/**
 * Copyright (c) 2026 Bosch Sensortec GmbH. All rights reserved.
 *
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/time.h"

#include "drivers.h"

// https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp581-ds004.pdf
// https://github.com/boschsensortec/BMP5_SensorAPI
// Based off `read_sensor_data_continuous_mode` example

#define BMP581_ADDR_LOW 0x46
#define BMP581_ADDR_HIGH 0x47

#define BMP581_REG_CHIP_ID 0x01
#define BMP581_REG_TEMP_DATA_XLSB 0x1D
#define BMP581_REG_DSP_CONFIG 0x30
#define BMP581_REG_DSP_IIR 0x31
#define BMP581_REG_OSR_CONFIG 0x36
#define BMP581_REG_ODR_CONFIG 0x37
#define BMP581_REG_CMD 0x7E

#define BMP581_CHIP_ID 0x50
#define BMP581_RESET_TRIGGER 0xB6

bool bmp581_exists(FusionDriver *self) {
    return check_devid(&self->bus, BMP581_ADDR_LOW, BMP581_ADDR_HIGH, BMP581_REG_CHIP_ID, BMP581_CHIP_ID,
                       driver_read_byte);
}

bool bmp581_init(FusionDriver *self) {
    bool success = true;
    // Reset device, this also puts us in standby mode
    success &= driver_write_byte(&self->bus, BMP581_REG_CMD, BMP581_RESET_TRIGGER);
    sleep_ms_blocking(2);
    // Enable pressure measurements (press_en), 16x pressure oversampling, 1x temperature oversampling
    success &= driver_write_byte(&self->bus, BMP581_REG_OSR_CONFIG, 0b01100000);
    // Enable press/temp compensation and the IIR filter for output press/temp data
    success &= driver_write_byte(&self->bus, BMP581_REG_DSP_CONFIG, 0b00101011);
    // Set both filter coefficients to 1
    success &= driver_write_byte(&self->bus, BMP581_REG_DSP_IIR, 0b00001001);
    // Set 120hz ODR and enable continuous output mode
    success &= driver_write_byte(&self->bus, BMP581_REG_ODR_CONFIG, 0b00100011);
    return success;
}

bool bmp581_read(FusionDriver *self, f32 data[]) {
    byte raw[6]; // 3 bytes for temp (XLSB, LSB, MSB), 3 bytes for press
    if (!driver_read(&self->bus, BMP581_REG_TEMP_DATA_XLSB, raw, sizeof(raw))) {
        return false;
    }

    u32 rawPress = (u32)(((u32)raw[5] << 16) | ((u32)raw[4] << 8) | (u32)raw[3]);
    data[0] = rawPress / 64.f; // Pressure in Pa

    u32 rawTempUnsigned = ((u32)raw[2] << 16U) | ((u32)raw[1] << 8U) | (u32)raw[0];
    i32 rawTemp;
    if ((rawTempUnsigned & 0x800000U) != 0U) {
        rawTemp = (i32)(rawTempUnsigned | 0xFF000000U);
    } else {
        rawTemp = (i32)rawTempUnsigned;
    }
    data[1] = rawTemp / 65536.f; // Temperature in °C
    data[2] = -1.f;              // BMP581 does not support humidity
    return true;
}

FusionDriver bmp581_driver = {
    .exists = bmp581_exists,
    .init = bmp581_init,
    .read = bmp581_read,
};
FusionDevice bmp581 = {
    .baro = &bmp581_driver,
    .name = "BMP581",
};
