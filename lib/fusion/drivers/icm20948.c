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

#include "sys/print.h"

#include "icm20948.h"

/* 
   ICM20948
   Datasheet: https://invensense.tdk.com/wp-content/uploads/2016/06/DS-000189-ICM-20948-v1.3.pdf
   Note: The BNO055 was the first sensor driver written so most comments are in that file. Only differing operations are documented here.

   A short note about the ICM20948: it's actually two dies on one chip, the ICM20948 for accel/gyro and the AK09916 for mag.
   This makes interacting with it a little weird, and since the AK09916 has a different i2c address, it's installed as a seperate sensor.
*/

const registerReadList_t ICM20948_WHO_AM_I_READ[] = {
    { .readFrom = ICM20948_WHO_AM_I, .numBytes = 1 }, __END_READ_DATA__
};

const registerReadList_t ICM20948_ACCEL_DATA_READ[] = {
    { .readFrom = ICM20948_ACCEL_XOUT_H, .numBytes = 6 }, __END_READ_DATA__
};

const registerReadList_t ICM20948_GYRO_DATA_READ[] = {
    { .readFrom = ICM20948_GYRO_XOUT_H, .numBytes = 6 }, __END_READ_DATA__
};

const registerWriteList_t ICM20948_CONFIGURE[] = {
    { ICM20948_PWR_MGMT_1, 0x80, 0x00 }, // Reset
    { ICM20948_PWR_MGMT_1, 0x09, 0x00 }, // Disable temp, auto-select clock source
    { ICM20948_USER_CTRL, 0x40, 0x00 }, // Enable FIFO
    { ICM20948_INT_PIN_CFG, 0x02, 0x00 }, // Enable bypass mode (for mag)
    { ICM20948_BANK_SEL, ICM20948_SEL_BANK_2, 0x00 },
    { ICM20948_GYRO_CONFIG_1, 0x2D, 0x00 }, // Gyro 1000 deg/s, 17Hz low-pass filter
    { ICM20948_GYRO_SMPLRT_DIV, 0x0A, 0x00 }, // Gyro 100Hz ODR
    { ICM20948_ACCEL_CONFIG_1, 0x2B, 0x00 }, // Accel ±4G, 17Hz low-pass filter
    { ICM20948_ACCEL_SMPLRT_DIV_2, 0x0A, 0x00 }, // Accel 100Hz±2 ODR
    { ICM20948_BANK_SEL, ICM20948_SEL_BANK_0, 0x00 },
    __END_WRITE_DATA__
};

#define ICM20948_COUNTS_PER_G 8192 // (datasheet pg. 12, we use ±4G range so ACCEL_FS=1)

#define ICM20948_COUNTS_PER_DPS 32.8 // (datasheet pg. 11, we use 1000 deg/s so GYRO_FS_SEL=2)

i8 ICM20948_init(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    i32 status;
    byte reg;
    printfbw(aahrs, "[ICM20948] initializing...");
    for (u32 i = 0; i < DRIVER_INIT_ATTEMPTS; i++) {
        if (shouldPrint.aahrs) printraw("attempt %d ", i);
        status = driver_read_register(&sensor->deviceInfo, sensor->addr, ICM20948_WHO_AM_I_READ[0].readFrom, ICM20948_WHO_AM_I_READ[0].numBytes, &reg);
        if (status == SENSOR_ERROR_NONE && reg == ICM20948_WHO_AM_I_EXPECTED) break;
    }
    if (shouldPrint.aahrs) printraw("\n");
    if (status != SENSOR_ERROR_NONE) {
        printfbw(aahrs, "[ICM20948] ERROR: address not acknowledged! (no/wrong device present?)");
        return status;
    }
    if (reg != ICM20948_WHO_AM_I_EXPECTED) {
        printfbw(aahrs, "[ICM20948] ERROR: could not verify chip!");
        return SENSOR_ERROR_INIT;
    }
    #if F_USING_ACCEL
        sfg->Accel.iWhoAmI = reg;
        sfg->Accel.iCountsPerg = ICM20948_COUNTS_PER_G;
        sfg->Accel.fCountsPerg = (float)ICM20948_COUNTS_PER_G;
        sfg->Accel.fgPerCount = 1.0f/ICM20948_COUNTS_PER_G;
    #endif
    #if F_USING_GYRO
        sfg->Gyro.iWhoAmI = reg;
        sfg->Gyro.iCountsPerDegPerSec = (i16)ICM20948_COUNTS_PER_DPS;
        sfg->Gyro.fDegPerSecPerCount = 1.0f/ICM20948_COUNTS_PER_DPS;
    #endif

    printfbw(aahrs, "[ICM20948] configuring...");
    status = driver_write_list(&sensor->deviceInfo, sensor->addr, ICM20948_CONFIGURE);
    #if F_USING_ACCEL
        sfg->Accel.isEnabled = true;
    #endif
    #if F_USING_GYRO
        sfg->Gyro.isEnabled = true;
    #endif
    printfbw(aahrs, "[ICM20948] sensor ready!");
    sensor->isInitialized = F_USING_ACCEL | F_USING_GYRO;
    return status;
}

#if F_USING_ACCEL
i8 ICM20948_read_accel(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    if (!(sensor->isInitialized & F_USING_ACCEL)) return SENSOR_ERROR_INIT;
    byte buf[6];
    i32 status = driver_read(&sensor->deviceInfo, sensor->addr, ICM20948_ACCEL_DATA_READ, buf);
    if (status == SENSOR_ERROR_NONE) {
        i16 sample[3];
        for (u32 i = 0; i < count_of(sample); i++) {
            // ICM20948 has high byte first so the shifting is slightly different (from BNO055)
            sample[i] = (buf[i * 2] << 8) | buf[i * 2 + 1];
        }
        conditionSample(sample);
        addToFifo((union FifoSensor*) &(sfg->Accel), ACCEL_FIFO_SIZE, sample);
    }
    return status;
}
#endif

#if F_USING_GYRO
i8 ICM20948_read_gyro(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    if (!(sensor->isInitialized & F_USING_GYRO)) return SENSOR_ERROR_INIT;
    byte buf[6];
    i32 status = driver_read(&sensor->deviceInfo, sensor->addr, ICM20948_GYRO_DATA_READ, buf);
    if (status == SENSOR_ERROR_NONE) {
        i16 sample[3];
        for (u32 i = 0; i < count_of(sample); i++) {
            sample[i] = (buf[i * 2] << 8) | buf[i * 2 + 1];
        }
        conditionSample(sample);
        addToFifo((union FifoSensor*) &(sfg->Gyro), GYRO_FIFO_SIZE, sample);
    }
    return status;
}
#endif

i8 ICM20948_read(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    i8 status_accel = 0, status_gyro = 0;
    #if F_USING_ACCEL
        status_accel = ICM20948_read_accel(sensor, sfg);
    #endif
    #if F_USING_GYRO
        status_gyro = ICM20948_read_gyro(sensor, sfg);
    #endif
    return (status_accel + status_gyro);
}
