/**
 * Copyright (c) 2020 Bosch Sensortec GmbH. All rights reserved.
 *
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

/**
 * This driver uses code from the BME280 datasheet.
 * Bosch Sensortec hereby disclaims any and all warranties and liabilities of any kind,
 * including without limitation warranties of non-infringement of intellectual property rights
 * or copyrights of any third party.
 * The information and example values provided are solely for illustrative purposes and do not
 * guarantee any specific conditions, characteristics, or evaluations regarding functionality,
 * performance, or infringement.
 */

#include <stdlib.h>
#include "platform/i2c.h"
#include "platform/time.h"

#include "drivers.h"

// https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf
// https://github.com/boschsensortec/BME280_SensorAPI

#define BME280_ADDR_LOW 0x76
#define BME280_ADDR_HIGH 0x77

#define BME280_REG_CALIB_00 0x88
#define BME280_REG_ID 0xD0
#define BME280_REG_RESET 0xE0
#define BME280_REG_CALIB_26 0xE1
#define BME280_REG_CTRL_HUM 0xF2
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_PRESS 0xF7

#define BME280_DEVICE_ID 0x60
#define BME280_RESET_TRIGGER 0xB6
#define BME280_MODE_NORMAL 0b11
#define BME280_OVERSAMPLE_X1 0b001
#define BME280_OVERSAMPLE_X2 0b010
#define BME280_OVERSAMPLE_X16 0b101

typedef struct BME280State {
    // Calibration (trim) parameters for measurement compensation
    u16 digT1, digP1;
    i16 digT2, digT3, digP2, digP3, digP4, digP5, digP6, digP7, digP8, digP9, digH2, digH4, digH5;
    byte digH1, digH3;
    signed char digH6;
    // Intermediate temperature value for compensation
    i32 tFine;
} BME280State;

/**
 * Create a new BME280State and populate its calibration parameters.
 * @param self FusionDriver instance to pull parameters from
 * @return a newly allocated BME280State with populated parameters, or NULL on failure
 * @note The caller is responsible for freeing the returned BME280State.
 */
static BME280State *create_bme280_state(FusionDriver *self) {
    // Fetch lower and upper calibration data from the device
    byte calibL[25], calibU[8];
    if (!i2c_read(ASDA, ASCL, self->addr, BME280_REG_CALIB_00, calibL, sizeof(calibL))) {
        return NULL;
    }
    if (!i2c_read(ASDA, ASCL, self->addr, BME280_REG_CALIB_26, calibU, sizeof(calibU))) {
        return NULL;
    }
    // Create a new state and populate its calibration parameters
    BME280State *state = malloc(sizeof(BME280State));
    if (!state) {
        return NULL;
    }
    state->digT1 = (calibL[1] << 8) | calibL[0];
    state->digT2 = (calibL[3] << 8) | calibL[2];
    state->digT3 = (calibL[5] << 8) | calibL[4];
    state->digP1 = (calibL[7] << 8) | calibL[6];
    state->digP2 = (calibL[9] << 8) | calibL[8];
    state->digP3 = (calibL[11] << 8) | calibL[10];
    state->digP4 = (calibL[13] << 8) | calibL[12];
    state->digP5 = (calibL[15] << 8) | calibL[14];
    state->digP6 = (calibL[17] << 8) | calibL[16];
    state->digP7 = (calibL[19] << 8) | calibL[18];
    state->digP8 = (calibL[21] << 8) | calibL[20];
    state->digP9 = (calibL[23] << 8) | calibL[22];
    state->digH1 = calibL[24];
    state->digH2 = (calibU[2] << 8) | calibU[1];
    state->digH3 = calibU[3];
    state->digH4 = (calibU[4] << 4) | (calibU[5] & 0x0F);
    state->digH5 = (calibU[6] << 4) | (calibU[5] >> 4);
    state->digH6 = calibU[7];
    return state;
}

/**
 * Compensate raw temperature data to a physical value.
 * @param raw raw temperature data
 * @param state BME280State instance with calibration parameters
 * @return compensated temperature
 * @note The data is returned as degrees Celsius with a resolution of 0.01°C (ex. 2500 for 25.00°C).
 */
static i32 compensate_temp(i32 raw, BME280State *state) {
    i32 var1 = ((((raw >> 3) - ((i32)state->digT1 << 1))) * ((i32)state->digT2)) >> 11;
    i32 var2 =
        (((((raw >> 4) - ((i32)state->digT1)) * ((raw >> 4) - ((i32)state->digT1))) >> 12) * ((i32)state->digT3)) >> 14;
    state->tFine = var1 + var2;
    return (state->tFine * 5 + 128) >> 8;
}

/**
 * Compensate raw pressure data to a physical value.
 * @param raw raw pressure data
 * @param state BME280State instance with calibration parameters
 * @return compensated pressure
 * @note The data is returned as Pascals in Q24.8 format (24 integer bits and 8 fractional bits).
 * (ex. 100000 = 100000/256 = 390.625 Pa)
 */
static u32 compensate_press(i32 raw, BME280State *state) {
    i64 var1 = ((i64)state->tFine) - 128000;
    i64 var2 = var1 * var1 * (i64)state->digP6;
    var2 = var2 + ((var1 * (i64)state->digP5) << 17);
    var2 = var2 + (((i64)state->digP4) << 35);
    var1 = ((var1 * var1 * (i64)state->digP3) >> 8) + ((var1 * (i64)state->digP2) << 12);
    var1 = (((((i64)1) << 47) + var1)) * ((i64)state->digP1) >> 33;
    if (var1 == 0) {
        return 0;
    }
    i64 p = 1048576 - raw;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((i64)state->digP9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((i64)state->digP8) * p) >> 19;
    return (u32)((p + var1 + var2) >> 8) + (((i64)state->digP7) << 4);
}

/**
 * Compensate raw humidity data to a physical value.
 * @param raw raw humidity data
 * @param state BME280State instance with calibration parameters
 * @return compensated humidity
 * @note The data is returned as %RH in Q22.10 format (22 integer bits and 10 fractional bits).
 * (ex. 47445 = 47445/1024 46.333%RH)
 */
static u32 compensate_hum(i32 raw, BME280State *state) {
    i32 v_x1_u32r = (state->tFine - ((i32)76800));
    v_x1_u32r =
        (((((raw << 14) - (((i32)state->digH4) << 20) - (((i32)state->digH5) * v_x1_u32r)) + ((i32)16384)) >> 15) *
         (((((((v_x1_u32r * ((i32)state->digH6)) >> 10) * (((v_x1_u32r * ((i32)state->digH3)) >> 11) + ((i32)32768))) >>
             10) +
            ((i32)2097152)) *
               ((i32)state->digH2) +
           8192) >>
          14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((i32)state->digH1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (u32)(v_x1_u32r >> 12);
}

bool bme280_exists(FusionDriver *self) {
    return check_devid(self, BME280_ADDR_LOW, BME280_ADDR_HIGH, BME280_REG_ID, BME280_DEVICE_ID);
}

bool bme280_init(FusionDriver *self) {
    bool success = true;
    // Reset device
    success &= i2c_write_byte(ASDA, ASCL, self->addr, BME280_REG_RESET, BME280_RESET_TRIGGER);
    sleep_ms_blocking(2);
    // Humidity x1 oversampling
    success &= i2c_write_bits(ASDA, ASCL, self->addr, BME280_REG_CTRL_HUM, 0b00000111, BME280_OVERSAMPLE_X1);
    // Pressure x16 oversampling, temperature x2 oversampling, normal mode
    success &= i2c_write_byte(ASDA, ASCL, self->addr, BME280_REG_CTRL_MEAS,
                              BME280_OVERSAMPLE_X2 << 5 | BME280_OVERSAMPLE_X16 << 2 | BME280_MODE_NORMAL);
    // Fetch calibration parameters for later readings
    BME280State *state = create_bme280_state(self);
    success &= state != NULL;
    self->context = state;
    return success;
}

bool bme280_read(FusionDriver *self, f32 data[]) {
    BME280State *state = (BME280State *)self->context;
    byte raw[8];
    if (!i2c_read(ASDA, ASCL, self->addr, BME280_REG_PRESS, raw, sizeof(raw))) {
        return false;
    }
    // Convert byte array to 20/16-bit signed adc values
    i32 adcPress = (raw[0] << 12) | (raw[1] << 4) | (raw[2] >> 4);
    i32 adcTemp = (raw[3] << 12) | (raw[4] << 4) | (raw[5] >> 4);
    i32 adcHum = (raw[6] << 8) | raw[7];
    // Compensate measurements and convert to floating point
    data[0] = compensate_press(adcPress, state) / 25600.f; // Pressure in Pa
    data[1] = compensate_temp(adcTemp, state) / 100.f;     // Temperature in °C
    data[2] = compensate_hum(adcHum, state) / 1024.f;      // Humidity in %RH
    return true;
}

void bme280_destroy(FusionDriver *self) {
    if (self->context) {
        free(self->context);
    }
}

FusionDriver bme280_driver = {
    .exists = bme280_exists,
    .init = bme280_init,
    .read = bme280_read,
    .destroy = bme280_destroy,
};
FusionDevice bme280 = {
    .baro = &bme280_driver,
    .name = "BME280",
};
