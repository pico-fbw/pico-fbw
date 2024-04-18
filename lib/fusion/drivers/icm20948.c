/*
 * Copyright 2018 Google Inc.
 *
 * This file utilizes code under the Apache-2.0 License. See "LICENSE" for details.
 */

/**
 * pico-fbw's IMU/fusion implementation is based on the mongoose-os's IMU library.
 * Check it out at https://github.com/mongoose-os-libs/imu
 */

#include <stdlib.h>

#include "compat.h"

#include "icm20948.h"

#define ICM20948_ACC_BASE_ODR 1125.f
#define ICM20948_GYRO_BASE_ODR 1100.f

static bool icm20948_change_bank(byte i2caddr, void *state, u8 bank_no) {
    ICM20948State *iud = (ICM20948State *)state;
    byte bank_addr = 0x00;

    if (!state)
        return false;

    if (bank_no == iud->current_bank_no)
        return true;

    switch (bank_no) {
        case 0:
            bank_addr = 0x00;
            break;
        case 1:
            bank_addr = 0x10;
            break;
        case 2:
            bank_addr = 0x20;
            break;
        case 3:
            bank_addr = 0x30;
            break;
    }

    if (mgos_i2c_write_reg_b(i2caddr, ICM20948_REG0_BANK_SEL, bank_addr)) {
        iud->current_bank_no = bank_no;
        return true;
    }
    return false;
}

static bool icm20948_detect(byte addr, void *state) {
    ICM20948State *iud = (ICM20948State *)state;
    i32 device_id;

    if (!state)
        return false;
    if (iud->accgyro_initialized)
        return true;

    device_id = mgos_i2c_read_reg_b(addr, ICM20948_REG0_WHO_AM_I);
    if (device_id == ICM20948_DEVID)
        return true;
    return false;
}

static bool icm20948_accgyro_create(byte i2caddr, void *state) {
    if (!icm20948_change_bank(i2caddr, state, 0))
        return false;

    // PWR_MGMNT_1: DEVICE_RESET=1; SLEEP=0; LP_EN=0; TEMP_DIS=0; CLKSEL=000;
    mgos_i2c_write_reg_b(i2caddr, ICM20948_REG0_PWR_MGMT_1, 0x80);
    mgos_usleep(11000);

    // PWR_MGMNT_1: DEVICE_RESET=0; SLEEP=0; LP_EN=0; TEMP_DIS=0; CLKSEL=001(auto clock source);
    mgos_i2c_write_reg_b(i2caddr, ICM20948_REG0_PWR_MGMT_1, 0x01);

    // INT_PIN_CFG: BYPASS_EN=1(enable slave bypass mode);
    mgos_i2c_write_reg_b(i2caddr, ICM20948_REG0_INT_PIN_CFG, 0x02);

    return true;
}

void *icm20948_state_create() {
    ICM20948State *iud;

    iud = calloc(1, sizeof(ICM20948State));
    if (!iud)
        return NULL;
    iud->accgyro_initialized = false;
    iud->current_bank_no = -1;
    return iud;
}

void *icm20948_state_destroy(void *state) {
    if (state)
        free(state);
    return NULL;
}

/* Accelerometer */

bool icm20948_acc_detect(byte addr, void *state) {
    return icm20948_detect(addr, state);
}

bool icm20948_acc_create(Accelerometer *dev, void *state) {
    ICM20948State *iud = (ICM20948State *)state;
    if (!dev)
        return false;

    // Only initialize the ICM20948 if gyro hasn't done so yet
    if (!iud->accgyro_initialized) {
        if (!icm20948_accgyro_create(dev->addr, state))
            return false;
        iud->accgyro_initialized = true;
    }

    if (!icm20948_change_bank(dev->addr, state, 2))
        return false;

    // REG2_ACCEL_CONFIG: ACCEL_DLPFCFG=101(12Hz); ACCEL_FS_SEL=10(8g); ACCEL_FCHOICE=1(Enable accel DLPF);
    // ACCEL_SMPLRT_DIV_1: ACCEL_SMPLRT_DIV=0000(MSB);
    // ACCEL_SMPLRT_DIV_2: ACCEL_SMPLRT_DIV=00001010(LSB);
    mgos_i2c_write_reg_b(dev->addr, ICM20948_REG2_ACCEL_CONFIG, 0x15);
    mgos_i2c_write_reg_b(dev->addr, ICM20948_REG2_ACCEL_SMPLRT_DIV_1, 0x00);
    mgos_i2c_write_reg_b(dev->addr, ICM20948_REG2_ACCEL_SMPLRT_DIV_2, 0x0a);
    dev->scale = 8.f / 32767.0f;

    return true;
}

bool icm20948_acc_read(Accelerometer *dev, void *state) {
    byte data[6];
    if (!dev)
        return false;

    if (!icm20948_change_bank(dev->addr, state, 0))
        return false;
    if (!mgos_i2c_read_reg_n(dev->addr, ICM20948_REG0_ACCEL_XOUT_H, 6, data))
        return false;

    dev->ax = (data[0] << 8) | (data[1]);
    dev->ay = (data[2] << 8) | (data[3]);
    dev->az = (data[4] << 8) | (data[5]);

    return true;
}

bool icm20948_acc_get_scale(Accelerometer *dev, void *state, float *scale) {
    u8 fs = 0;

    if (!scale)
        return false;

    if (!icm20948_change_bank(dev->addr, state, 2))
        return false;
    if (!mgos_i2c_getbits_reg_b(dev->addr, ICM20948_REG2_ACCEL_CONFIG, 1, 2, &fs))
        return false;

    switch (fs) {
        case 0:
            *scale = 2;
            break;
        case 1:
            *scale = 4;
            break;
        case 2:
            *scale = 8;
            break;
        case 3:
            *scale = 16;
            break;
    }

    return true;
}

bool icm20948_acc_set_scale(Accelerometer *dev, void *state, float scale) {
    u8 fs = 0;

    if (scale <= 2) {
        fs = 0;
        scale = 2.f;
    } else if (scale <= 4) {
        fs = 1;
        scale = 4.f;
    } else if (scale <= 8) {
        fs = 2;
        scale = 8.f;
    } else if (scale <= 16) {
        fs = 3;
        scale = 16.f;
    } else
        return false;

    if (!icm20948_change_bank(dev->addr, state, 2))
        return false;
    if (!mgos_i2c_setbits_reg_b(dev->addr, ICM20948_REG2_ACCEL_CONFIG, 1, 2, fs))
        return false;
    dev->opts.scale = scale;
    dev->scale = dev->opts.scale / 32767.0f;

    return true;
}

bool icm20948_acc_get_odr(Accelerometer *dev, void *state, float *odr) {
    u16 div = 0;

    if (!odr)
        return false;

    if (!icm20948_change_bank(dev->addr, state, 2))
        return false;
    if (!mgos_i2c_read_reg_n(dev->addr, ICM20948_REG2_ACCEL_SMPLRT_DIV_1, 2, (u8 *)&div))
        return false;
    div = (div & 0x00ff) << 8 | (div & 0xff00) >> 8;
    // ODR is computed as follows:
    // 1.125 kHz/(1+ACCEL_SMPLRT_DIV[11:0])
    *odr = ICM20948_ACC_BASE_ODR / (div + 1);
    return *odr >= 0;
}

bool icm20948_acc_set_odr(Accelerometer *dev, void *state, float odr) {
    u16 div = 0;

    if (odr <= 0)
        return false;

    div = (ICM20948_ACC_BASE_ODR / odr) - 1;
    if (div > 0xfff)
        return false; // Not feasible

    if (!icm20948_change_bank(dev->addr, state, 2))
        return false;
    div = (div & 0x00ff) << 8 | (div & 0xff00) >> 8;
    if (!mgos_i2c_write_reg_n(dev->addr, ICM20948_REG2_ACCEL_SMPLRT_DIV_1, 2, (u8 *)&div))
        return false;
    dev->opts.odr = odr;

    return true;
}

/* Gyroscope */

bool icm20948_gyro_detect(byte addr, void *state) {
    return icm20948_detect(addr, state);
}

bool icm20948_gyro_create(Gyroscope *dev, void *state) {
    ICM20948State *iud = (ICM20948State *)state;
    if (!dev)
        return false;

    // Only initialize the ICM20948 if acc hasn't done so yet
    if (!iud->accgyro_initialized) {
        if (!icm20948_accgyro_create(dev->addr, state))
            return false;
        iud->accgyro_initialized = true;
    }

    if (!icm20948_change_bank(dev->addr, state, 2))
        return false;

    // GYRO_CONFIG_1: GYRO_DLPFCFG=101(12Hz); GYRO_FS_SEL=11(2000dps); GYRO_FCHOICE=1(Enable gyro DLPF);
    // GYRO_SMPLRT_DIV: GYRO_SMPLRT_DIV=00000000;
    mgos_i2c_write_reg_b(dev->addr, ICM20948_REG2_GYRO_CONFIG_1, 0x2F);
    mgos_i2c_write_reg_b(dev->addr, ICM20948_REG2_GYRO_SMPLRT_DIV, 0x0a);
    dev->scale = 2000 / 32767.0f;

    return true;
}

bool icm20948_gyro_read(Gyroscope *dev, void *state) {
    byte data[6];
    if (!dev)
        return false;

    if (!icm20948_change_bank(dev->addr, state, 0))
        return false;
    if (!mgos_i2c_read_reg_n(dev->addr, ICM20948_REG0_GYRO_XOUT_H, 6, data))
        return false;
    dev->gx = (data[0] << 8) | (data[1]);
    dev->gy = (data[2] << 8) | (data[3]);
    dev->gz = (data[4] << 8) | (data[5]);

    return true;
}

bool icm20948_gyro_get_scale(Gyroscope *dev, void *state, float *scale) {
    u8 fs = 0;
    if (!scale)
        return false;

    if (!icm20948_change_bank(dev->addr, state, 2))
        return false;
    if (!mgos_i2c_getbits_reg_b(dev->addr, ICM20948_REG2_GYRO_CONFIG_1, 1, 2, &fs))
        return false;
    switch (fs) {
        case 0:
            *scale = 250;
            break;
        case 1:
            *scale = 500;
            break;
        case 2:
            *scale = 1000;
            break;
        case 3:
            *scale = 2000;
            break;
    }

    return true;
}

bool icm20948_gyro_set_scale(Gyroscope *dev, void *state, float scale) {
    u8 fs = 0;

    if (scale <= 250) {
        fs = 0;
        scale = 250.f;
    } else if (scale <= 500) {
        fs = 1;
        scale = 500.f;
    } else if (scale <= 1000) {
        fs = 2;
        scale = 1000.f;
    } else if (scale <= 2000) {
        fs = 3;
        scale = 2000.f;
    } else
        return false;

    if (!icm20948_change_bank(dev->addr, state, 2))
        return false;
    if (!mgos_i2c_setbits_reg_b(dev->addr, ICM20948_REG2_GYRO_CONFIG_1, 1, 2, fs))
        return false;
    dev->opts.scale = scale;
    dev->scale = dev->opts.scale / 32767.0f;

    return true;
}

bool icm20948_gyro_get_odr(Gyroscope *dev, void *state, float *odr) {
    u8 div = 0;
    if (!odr)
        return false;

    if (!icm20948_change_bank(dev->addr, state, 2))
        return false;
    if (!mgos_i2c_read_reg_n(dev->addr, ICM20948_REG2_GYRO_SMPLRT_DIV, 1, &div))
        return false;

    // ODR is computed as follows:
    // 1.1 kHz/(1+GYRO_SMPLRT_DIV[7:0])
    *odr = ICM20948_GYRO_BASE_ODR / (div + 1);
    return *odr >= 0;
}

bool icm20948_gyro_set_odr(Gyroscope *dev, void *state, float odr) {
    u8 div = 0;
    if (odr <= 0 || odr > ICM20948_GYRO_BASE_ODR)
        return false;

    div = (ICM20948_GYRO_BASE_ODR / odr) - 1;
    if (!icm20948_change_bank(dev->addr, state, 2))
        return false;
    if (!mgos_i2c_write_reg_b(dev->addr, ICM20948_REG2_GYRO_SMPLRT_DIV, div))
        return false;
    dev->opts.odr = odr;

    return true;
}

/* Magnetometer */
// The magnetometer in the ICM20948 is a bit interesting.
// It's actually on a seperate die and as such shows up as an entirely different device on the I2C bus.

bool icm20948_mag_detect(byte addr, void *state) {
    i32 device_id;

    device_id = mgos_i2c_read_reg_b(addr, ICM20948_WHO_AM_I_M);
    return device_id == ICM20948_DEVID_M;

    (void)state;
}

bool icm20948_mag_create(Magnetometer *dev, void *state) {
    if (!dev)
        return false;

    // CNTL3: SRST=1;
    mgos_i2c_write_reg_b(dev->addr, ICM20948_CNTL3_M, 0x01);

    // CNTL2: 01000(MODE4, 100Hz);
    mgos_i2c_write_reg_b(dev->addr, ICM20948_CNTL2_M, 0x08);

    dev->scale = 22.f / 32768.0;
    dev->bias[0] = 1.0;
    dev->bias[1] = 1.0;
    dev->bias[2] = 1.0;
    return true;

    (void)state;
}

bool icm20948_mag_read(Magnetometer *dev, void *state) {
    byte data[6];
    if (!dev)
        return false;

    if (!mgos_i2c_read_reg_n(dev->addr, ICM20948_HXL_M, 6, data))
        return false;

    // It is required to read ST2 register after data reading.
    mgos_i2c_read_reg_b(dev->addr, ICM20948_ST2_M);

    dev->mx = (data[1] << 8) | (data[0]);
    dev->my = (data[3] << 8) | (data[2]);
    dev->mz = (data[5] << 8) | (data[4]);
    return true;

    (void)state;
}

bool icm20948_mag_get_odr(Magnetometer *dev, void *state, float *odr) {
    i16 mode;
    if (!odr)
        return false;

    mode = mgos_i2c_read_reg_b(dev->addr, ICM20948_CNTL2_M);
    if (mode == -1)
        return false;

    switch (mode) {
        case 0x02:
            *odr = 10;
            break;
        case 0x04:
            *odr = 20;
            break;
        case 0x06:
            *odr = 50;
            break;
        case 0x08:
            *odr = 100;
            break;
        default:
            return false;
    }

    return true;
    (void)state;
}

bool icm20948_mag_set_odr(Magnetometer *dev, void *state, float odr) {
    u8 mode = 0;

    if (odr <= 10) {
        mode = 0x02;
    } else if (odr <= 20) {
        mode = 0x04;
    } else if (odr <= 50) {
        mode = 0x06;
    } else if (odr <= 100) {
        mode = 0x08;
    } else
        return false;
    if (!mgos_i2c_write_reg_b(dev->addr, ICM20948_CNTL2_M, mode)) {
        return false;
    }

    return false;
    (void)state;
}
