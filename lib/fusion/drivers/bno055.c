/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright (C) 2015 - 2016 Bosch Sensortec GmbH
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

#include "platform/time.h"

#include "sys/boot.h"
#include "sys/log.h"
#include "sys/print.h"

#include "bno055.h"

/* 
   BNO055 
   Datasheet: https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bno055-ds000.pdf
*/

const registerReadList_t BNO055_CHIP_ID_READ[] = {
    { .readFrom = BNO055_CHIP_ID, .numBytes = 1 }, __END_READ_DATA__
};

const registerReadList_t BNO055_ACCEL_DATA_READ[] = {
    { .readFrom = BNO055_ACCEL_DATA_X_LSB, .numBytes = 6 }, __END_READ_DATA__
};

const registerReadList_t BNO055_MAG_DATA_READ[] = {
    { .readFrom = BNO055_MAG_DATA_X_LSB, .numBytes = 6 }, __END_READ_DATA__
};

const registerReadList_t BNO055_GYRO_DATA_READ[] = {
    { .readFrom = BNO055_GYRO_DATA_X_LSB, .numBytes = 6 }, __END_READ_DATA__
};

const registerWriteList_t BNO055_CONFIGURE[] = {
    { BNO055_SYS_TRIGGER, 0x00, 0x00 }, // Use internal oscillator
    { BNO055_PWR_MODE, BNO055_POWER_MODE_NORMAL, 0x00 },
    { BNO055_UNIT_SEL, 0x01, 0x00 }, // Select units (Windows orientation, °C, °, °/s, mg)
    /* TODO: these configs were removed as fusion appears to operate better without them, but some may still be relavent,
     * I'm just not entirely sure of the effects.
    { BNO055_PAGE_ID, BNO055_PAGE_ONE, 0x00 }, // Switch to page 1
    { BNO055_ACCEL_CONFIG, 0b00010000, 0x00 }, // Accel config (normal opmode, 125Hz bandwidth, ±2G range)
    { BNO055_GYRO_CONFIG, 0b00010011, 0x00 }, // Gyro config (normal opmode, 116Hz bandwidth, ±250°/s range)
    { BNO055_MAG_CONFIG, 0b0010111, 0x00 }, // Mag config (normal power mode, enhanced opmode, 30Hz ODR)
    { BNO055_PAGE_ID, BNO055_PAGE_ZERO, 0x00 }, // Switch back to page 0
    */
    { BNO055_OPR_MODE, BNO055_OPERATION_MODE_AMG, 0x00 }, // Set operation mode to AMG (accel, mag, gyro) for raw data
    __END_WRITE_DATA__
};

// We use mG (milli-G) units of acceleration which are 1:1 (datasheet pg. 33) and there are 1000 mG in one G, thus 1000 CPG
#define BNO055_COUNTS_PER_G 1000

#define BNO055_COUNTS_PER_UT 16 // (datasheet pg. 34)

#define BNO055_COUNTS_PER_DPS 16 // (datasheet pg. 35)

i8 BNO055_init(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    i32 status;
    byte reg;
    if (boot_type() != BOOT_COLD) {
        // The BNO055 has a bug where it can't be soft-reset with an I2C command, it must either be hard-reset with the RESET pin
        // (which we don't have access to) or simply power cycled, so we must force the user to power-cycle the device
        // because it's likely still running from a previous soft-reboot
        log_message(FATAL, "Please reboot.", 1000, 0, true);
        return SENSOR_ERROR_INIT;
    }
    // Wait if BNO055 isn't ready yet (takes 850ms)
    while (time_us() < (850 * 1000));
    // Check that the sensor comms are okay and that it's a BNO055
    printfbw(aahrs, "[BNO055] initializing...");
    for (u32 i = 0; i < DRIVER_INIT_ATTEMPTS; i++) {
        if (shouldPrint.aahrs) printraw("attempt %lu ", i);
        status = driver_read_register(&sensor->deviceInfo, sensor->addr, BNO055_CHIP_ID_READ[0].readFrom, BNO055_CHIP_ID_READ[0].numBytes, &reg);
        if (status == SENSOR_ERROR_NONE && reg == BNO055_CHIP_ID_EXPECTED) break;
    }
    if (shouldPrint.aahrs) printraw("\n");
    if (status != SENSOR_ERROR_NONE) {
        printfbw(aahrs, "[BNO055] ERROR: address not acknowledged! (no/wrong device present?)");
        return status;
    }
    if (reg != BNO055_CHIP_ID_EXPECTED) {
        printfbw(aahrs, "[BNO055] ERROR: could not verify chip!");
        return SENSOR_ERROR_INIT;
    }
    // Set up sensor data for fusion algorithms later
    #if F_USING_ACCEL
        sfg->Accel.iWhoAmI = reg;
        sfg->Accel.iCountsPerg = BNO055_COUNTS_PER_G;
        sfg->Accel.fCountsPerg = (float)BNO055_COUNTS_PER_G;
        sfg->Accel.fgPerCount = 1.0f/BNO055_COUNTS_PER_G;
    #endif
    #if F_USING_MAG
        sfg->Mag.iWhoAmI = reg;
        sfg->Mag.iCountsPeruT = BNO055_COUNTS_PER_UT;
        sfg->Mag.fCountsPeruT = (float)BNO055_COUNTS_PER_UT;
        sfg->Mag.fuTPerCount = 1.0f/BNO055_COUNTS_PER_UT;
    #endif
    #if F_USING_GYRO
        sfg->Gyro.iWhoAmI = reg;
        sfg->Gyro.iCountsPerDegPerSec = BNO055_COUNTS_PER_DPS;
        sfg->Gyro.fDegPerSecPerCount = 1.0f/BNO055_COUNTS_PER_DPS;
    #endif

    // Configure sensor
    printfbw(aahrs, "[BNO055] configuring...");
    status = driver_write_list(&sensor->deviceInfo, sensor->addr, BNO055_CONFIGURE);
    sleep_ms_blocking(200);
    // Read back mode change
    byte mode;
    driver_read_register(&sensor->deviceInfo, sensor->addr, BNO055_OPR_MODE, 1, &mode);
    if (mode != BNO055_OPERATION_MODE_AMG) {
        printfbw(aahrs, "[BNO055] ERROR: could not configure sensor mode %02X (still %02X)!", BNO055_OPERATION_MODE_AMG, mode);
        return SENSOR_ERROR_INIT;
    }
    #if F_USING_ACCEL
        sfg->Accel.isEnabled = true;
    #endif
    #if F_USING_MAG
        sfg->Mag.isEnabled = true;
    #endif
    #if F_USING_GYRO
        sfg->Gyro.isEnabled = true;
    #endif
    printfbw(aahrs, "[BNO055] sensor ready!");
    sensor->isInitialized = F_USING_ACCEL | F_USING_MAG | F_USING_GYRO;
    return status;
}

#if F_USING_ACCEL
i8 BNO055_read_accel(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    if (!(sensor->isInitialized & F_USING_ACCEL)) return SENSOR_ERROR_INIT;
    byte buf[6];
    i32 status = driver_read(&sensor->deviceInfo, sensor->addr, BNO055_ACCEL_DATA_READ, buf);
    if (status == SENSOR_ERROR_NONE) {
        i16 sample[3];
        // Shift LSB and MSB together
        for (u32 i = 0; i < count_of(sample); i++) {
            sample[i] = (buf[i * 2 + 1] << 8) | buf[i * 2]; // Casting breaks things here for some reason?
        }
        // Normalize data and add to FIFO
        conditionSample(sample);
        addToFifo((union FifoSensor*) &(sfg->Accel), ACCEL_FIFO_SIZE, sample);
    }
    return status;
}
#endif

#if F_USING_MAG
i8 BNO055_read_mag(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    if (!(sensor->isInitialized & F_USING_MAG)) return SENSOR_ERROR_INIT;
    byte buf[6];
    i32 status = driver_read(&sensor->deviceInfo, sensor->addr, BNO055_MAG_DATA_READ, buf);
    if (status == SENSOR_ERROR_NONE) {
        i16 sample[3];
        for (u32 i = 0; i < count_of(sample); i++) {
            sample[i] = (buf[i * 2 + 1] << 8) | buf[i * 2];
        }
        conditionSample(sample);
        addToFifo((union FifoSensor*) &(sfg->Mag), MAG_FIFO_SIZE, sample);
    }
    return status;
}
#endif

#if F_USING_GYRO
i8 BNO055_read_gyro(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    if (!(sensor->isInitialized & F_USING_GYRO)) return SENSOR_ERROR_INIT;
    byte buf[6];
    i32 status = driver_read(&sensor->deviceInfo, sensor->addr, BNO055_GYRO_DATA_READ, buf);
    if (status == SENSOR_ERROR_NONE) {
        i16 sample[3];
        for (u32 i = 0; i < count_of(sample); i++) {
            sample[i] = (buf[i * 2 + 1] << 8) | buf[i * 2];
        }
        conditionSample(sample);
        addToFifo((union FifoSensor*) &(sfg->Gyro), GYRO_FIFO_SIZE, sample);
    }
    return status;
}
#endif

i8 BNO055_read(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    i8 status_accel = 0, status_mag = 0, status_gyro = 0;
    #if F_USING_ACCEL
        status_accel = BNO055_read_accel(sensor, sfg);
    #endif
    #if F_USING_MAG
        status_mag = BNO055_read_mag(sensor, sfg);
    #endif
    #if F_USING_GYRO
        status_gyro = BNO055_read_gyro(sensor, sfg);
    #endif
    return (status_accel + status_mag + status_gyro);
}
