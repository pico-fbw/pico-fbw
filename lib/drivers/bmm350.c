/**
 * Copyright (c) 2023 Bosch Sensortec GmbH. All rights reserved.
 *
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <stdlib.h>
#include "platform/helpers.h"
#include "platform/i2c.h"
#include "platform/time.h"

#include "drivers.h"

// https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmm350-ds001.pdf
// https://github.com/boschsensortec/BMM350_SensorAPI

#define BMM350_ADDR_LOW 0x14
#define BMM350_ADDR_HIGH 0x15

#define BMM350_REG_ID 0x00
#define BMM350_REG_PMU_CMD 0x06
#define BMM350_REG_MAG_X_LSB 0x31
#define BMM350_REG_OTP_CMD 0x50
#define BMM350_REG_OTP_DATA_MSB 0x52
#define BMM350_REG_OTP_STATUS 0x55
#define BMM350_REG_CMD 0x7E

#define BMM350_DEVICE_ID 0x33
#define BMM350_RESET_TRIGGER 0xB6
#define BMM350_OTP_CMD_DIR_READ 0x20
#define BMM350_OTP_CMD_POWEROFF 0x80
#define BMM350_OTP_WORD_ADDR_MASK 0x1F
#define BMM350_OTP_STATUS_ERROR_MASK 0xE0
#define BMM350_SIGNED_8_BIT 8
#define BMM350_SIGNED_12_BIT 12
#define BMM350_SIGNED_16_BIT 16
#define BMM350_SIGNED_21_BIT 21
#define BMM350_SIGNED_24_BIT 24
#define BMM350_BXY_SENS 14.55f
#define BMM350_BZ_SENS 9.0f
#define BMM350_TEMP_SENS 0.00204f
#define BMM350_INA_XY_GAIN_TRGT 19.46f
#define BMM350_INA_Z_GAIN_TRGT 31.0f
#define BMM350_ADC_GAIN (1 / 1.5f)
#define BMM350_LUT_GAIN 0.714607238769531f
#define BMM350_POWER (f32)(1000000.0 / 1048576.0)

// OTP word addresses
#define BMM350_TEMP_OFF_SENS 0x0D
#define BMM350_MAG_OFFSET_X 0x0E
#define BMM350_MAG_OFFSET_Y 0x0F
#define BMM350_MAG_OFFSET_Z 0x10
#define BMM350_MAG_SENS_X 0x10
#define BMM350_MAG_SENS_Y 0x11
#define BMM350_MAG_SENS_Z 0x11
#define BMM350_MAG_TCO_X 0x12
#define BMM350_MAG_TCO_Y 0x13
#define BMM350_MAG_TCO_Z 0x14
#define BMM350_MAG_TCS_X 0x12
#define BMM350_MAG_TCS_Y 0x13
#define BMM350_MAG_TCS_Z 0x14
#define BMM350_MAG_DUT_T_0 0x18
#define BMM350_CROSS_X_Y 0x15
#define BMM350_CROSS_Y_X 0x15
#define BMM350_CROSS_Z_X 0x16
#define BMM350_CROSS_Z_Y 0x16

typedef struct BMM350State {
    f32 offset[4]; // x, y, z, temperature offsets
    f32 sens[4];   // x, y, z, temperature sensitivities
    f32 tco[3];    // x, y, z temperature coefficients
    f32 tcs[3];    // x, y, z temperature coefficients for sensitivity
    f32 dutT0;
    f32 crossXY, crossYX, crossZX, crossZY;
} BMM350State;

/**
 * Convert a raw value from a register to a signed integer.
 * @param raw raw value read from the register
 * @param num_bits number of bits used to represent the value
 * @return signed integer value
 */
static i32 fix_sign(uint32_t raw, i8 num_bits) {
    i32 power = 0;
    i32 ret;
    switch (num_bits) {
        case BMM350_SIGNED_8_BIT:
            power = 128; // 2^7
            break;
        case BMM350_SIGNED_12_BIT:
            power = 2048; // 2^11
            break;
        case BMM350_SIGNED_16_BIT:
            power = 32768; // 2^15
            break;
        case BMM350_SIGNED_21_BIT:
            power = 1048576; // 2^20
            break;
        case BMM350_SIGNED_24_BIT:
            power = 8388608; // 2^23
            break;
        default:
            power = 0;
            break;
    }
    ret = (i32)raw;
    if (ret >= power) {
        ret = ret - (power * 2);
    }
    return ret;
}

static word read_otp_word(FusionDriver *self, byte reg) {
    // Command the OTP to read the word at the given address
    byte cmd = BMM350_OTP_CMD_DIR_READ | (reg & BMM350_OTP_WORD_ADDR_MASK);
    if (!i2c_write_byte(ASDA, ASCL, self->addr, BMM350_REG_OTP_CMD, cmd)) {
        return 0;
    }
    // Wait for the OTP to return data
    Timestamp timeout = timestamp_in_ms(10);
    byte otpStatus = UINT8_MAX;
    while (otpStatus == 0 || timestamp_reached(&timeout)) {
        otpStatus = i2c_read_byte(ASDA, ASCL, self->addr, BMM350_REG_OTP_STATUS) & BMM350_OTP_STATUS_ERROR_MASK;
    }
    if (otpStatus != 0) {
        return 0;
    }
    // Word is now ready to be read
    byte data[2];
    if (!i2c_read(ASDA, ASCL, self->addr, BMM350_REG_OTP_DATA_MSB, data, sizeof(data))) {
        return 0;
    }
    return (word)((data[0] << 8) | data[1]);
}

/**
 * Create a new BMM350State and populate its fields.
 * @param self FusionDriver instance to pull parameters from
 * @return a newly allocated BMM350State with populated fields, or NULL on failure
 * @note The caller is responsible for freeing the returned state.
 */
static BMM350State *create_bmm350_state(FusionDriver *self) {
    // Read OTP for calibration data
    word otp[32];
    for (u32 i = 0; i < count_of(otp); i++) {
        otp[i] = read_otp_word(self, i);
        if (otp[i] == 0) {
            return NULL; // Failed to read OTP word
        }
    }
    BMM350State *state = malloc(sizeof(BMM350State));
    if (!state) {
        return NULL;
    }
    // Populate the state with calibration data from OTP
    state->offset[0] = fix_sign((otp[BMM350_MAG_OFFSET_X] & 0x0FFF), BMM350_SIGNED_12_BIT);
    state->offset[1] = fix_sign(((otp[BMM350_MAG_OFFSET_X] & 0xF000) >> 4) + (otp[BMM350_MAG_OFFSET_Y] & 0x00FF),
                                BMM350_SIGNED_12_BIT);
    state->offset[2] =
        fix_sign((otp[BMM350_MAG_OFFSET_Y] & 0x0F00) + (otp[BMM350_MAG_OFFSET_Z] & 0x00FF), BMM350_SIGNED_12_BIT);
    state->offset[3] = fix_sign((otp[BMM350_TEMP_OFF_SENS] & 0x00FF), BMM350_SIGNED_8_BIT) / 5.0f;
    state->sens[0] = fix_sign((otp[BMM350_MAG_SENS_X] & 0xFF00) >> 8, BMM350_SIGNED_8_BIT) / 256.0f;
    state->sens[1] = (fix_sign((otp[BMM350_MAG_SENS_Y] & 0x00FF), BMM350_SIGNED_8_BIT) / 256.0f) + 0.01f;
    state->sens[2] = fix_sign((otp[BMM350_MAG_SENS_Z] & 0xFF00) >> 8, BMM350_SIGNED_8_BIT) / 256.0f;
    state->sens[3] = fix_sign((otp[BMM350_TEMP_OFF_SENS] & 0xFF00), BMM350_SIGNED_8_BIT) / 512.0f;
    state->tco[0] = fix_sign((otp[BMM350_MAG_TCO_X] & 0x00FF), BMM350_SIGNED_8_BIT) / 32.0f;
    state->tco[1] = fix_sign((otp[BMM350_MAG_TCO_Y] & 0x00FF), BMM350_SIGNED_8_BIT) / 32.0f;
    state->tco[2] = fix_sign((otp[BMM350_MAG_TCO_Z] & 0x00FF), BMM350_SIGNED_8_BIT) / 32.0f;
    state->tcs[0] = fix_sign((otp[BMM350_MAG_TCS_X] & 0xFF00) >> 8, BMM350_SIGNED_8_BIT) / 16384.0f;
    state->tcs[1] = fix_sign((otp[BMM350_MAG_TCS_Y] & 0xFF00) >> 8, BMM350_SIGNED_8_BIT) / 16384.0f;
    state->tcs[2] = (fix_sign((otp[BMM350_MAG_TCS_Z] & 0xFF00) >> 8, BMM350_SIGNED_8_BIT) / 16384.0f) - 0.0001f;
    state->dutT0 = (fix_sign(otp[BMM350_MAG_DUT_T_0], BMM350_SIGNED_16_BIT) / 512.0f) + 23.0f;
    state->crossXY = fix_sign((otp[BMM350_CROSS_X_Y] & 0x00FF), BMM350_SIGNED_8_BIT) / 800.0f;
    state->crossYX = fix_sign((otp[BMM350_CROSS_Y_X] & 0xFF00) >> 8, BMM350_SIGNED_8_BIT) / 800.0f;
    state->crossZX = fix_sign((otp[BMM350_CROSS_Z_X] & 0x00FF), BMM350_SIGNED_8_BIT) / 800.0f;
    state->crossZY = fix_sign((otp[BMM350_CROSS_Z_Y] & 0xFF00) >> 8, BMM350_SIGNED_8_BIT) / 800.0f;
    return state;
}

bool bmm350_exists(FusionDriver *self) {
    return check_devid(self, BMM350_ADDR_LOW, BMM350_ADDR_HIGH, BMM350_REG_ID, BMM350_DEVICE_ID);
}

bool bmm350_init(FusionDriver *self) {
    bool success = true;
    // Soft reset device
    success &= i2c_write_byte(ASDA, ASCL, self->addr, BMM350_REG_CMD, BMM350_RESET_TRIGGER);
    sleep_ms_blocking(24);
    // Create state (needs to be done before OTP power off)
    BMM350State *state = create_bmm350_state(self);
    success &= state != NULL;
    self->context = state;
    // Power off OTP
    success &= i2c_write_byte(ASDA, ASCL, self->addr, BMM350_REG_OTP_CMD, BMM350_OTP_CMD_POWEROFF);
    // Enter normal mode
    success &= i2c_write_byte(ASDA, ASCL, self->addr, BMM350_REG_PMU_CMD, 0x01);
    return success;
}

bool bmm350_read(FusionDriver *self, f32 data[]) {
    BMM350State *state = (BMM350State *)self->context;
    byte raw[12];
    if (!i2c_read(ASDA, ASCL, self->addr, BMM350_REG_MAG_X_LSB, raw, sizeof(raw))) {
        return false;
    }
    u32 uncomp[] = {
        raw[0] + ((u32)raw[1] << 8) +
            ((u32)raw[2] << 16) *
                (BMM350_POWER / (BMM350_BXY_SENS * BMM350_INA_XY_GAIN_TRGT * BMM350_ADC_GAIN * BMM350_LUT_GAIN)),
        raw[3] + ((u32)raw[4] << 8) +
            ((u32)raw[5] << 16) *
                (BMM350_POWER / (BMM350_BXY_SENS * BMM350_INA_XY_GAIN_TRGT * BMM350_ADC_GAIN * BMM350_LUT_GAIN)),
        raw[6] + ((u32)raw[7] << 8) +
            ((u32)raw[8] << 16) *
                (BMM350_POWER / (BMM350_BXY_SENS * BMM350_INA_Z_GAIN_TRGT * BMM350_ADC_GAIN * BMM350_LUT_GAIN)),
        raw[9] + ((u32)raw[10] << 8) +
            ((u32)raw[11] << 16) / (BMM350_TEMP_SENS * BMM350_ADC_GAIN * BMM350_LUT_GAIN * 1048576),
    }; // x, y, z, temperature
    // Compensate raw magnetic data
    for (u32 i = 0; i < 3; i++) {
        uncomp[i] *= 1 + state->sens[i];
        uncomp[i] += state->offset[i];
        uncomp[i] += state->tco[i] * (uncomp[3] - state->dutT0);
        uncomp[i] /= 1 + state->tcs[i] * (uncomp[3] - state->dutT0);
    }
    data[0] = (uncomp[0] - state->crossXY * uncomp[1]) / (1 - state->crossYX * state->crossXY);
    data[1] = (uncomp[1] - state->crossYX * uncomp[0]) / (1 - state->crossYX * state->crossXY);
    data[2] = (uncomp[2] + (uncomp[0] * (state->crossYX * state->crossZY - state->crossZX) -
                            uncomp[1] * (state->crossZY - state->crossXY * state->crossZX)) /
                               (1 - state->crossYX * state->crossXY));
    return true;
}

void bmm350_destroy(FusionDriver *self) {
    if (self->context) {
        free(self->context);
    }
}

FusionDriver bmm350_driver = {
    .exists = bmm350_exists,
    .init = bmm350_init,
    .read = bmm350_read,
    .destroy = bmm350_destroy,
};
const FusionDevice bmm350 = {
    .mag = &bmm350_driver,
    .name = "BMM350",
};
