/*
 * Copyright 2018 Google Inc.
 *
 * This file utilizes code under the Apache-2.0 License. See "LICENSE" for details.
 */

/**
 * pico-fbw's IMU/fusion implementation is based on the mongoose-os's IMU library.
 * Check it out at https://github.com/mongoose-os-libs/imu
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <string.h>

#include "drivers/drivers.h"
#include "drivers/icm20948.h"

#include "sys/print.h"

#include "fusion.h"

AccelerometerDetails accelerometers[] = {
    {
        "ICM20948",
        {0x69, 0x68},
        {.create = icm20948_acc_create,
         .read = icm20948_acc_read,
         .get_odr = icm20948_acc_get_odr,
         .set_odr = icm20948_acc_set_odr,
         .get_scale = icm20948_acc_get_scale,
         .set_scale = icm20948_acc_set_scale},
        icm20948_acc_detect,
        icm20948_state_create,
        icm20948_state_destroy,
    },
};

bool fusion_accelerometer_find(IMU *imu, const AccelerometerOptions *opts) {
    if (!imu)
        return false;
    imu->acc->opts = *opts;

    for (u32 i = 0; i < count_of(accelerometers); i++) {
        // (possibly) Temporarily assign the current accelerometer's device driver into the global imu struct
        imu->acc = &(accelerometers[i].device);
        bool detected = false;
        // If we need to create a state for the accelerometer, do so now
        if (!imu->state && accelerometers[i].create_state) {
            imu->state = accelerometers[i].create_state();
            if (!imu->state) {
                printfbw(aahrs, "ERROR: could not create user data for accelerometer \"%s\"", accelerometers[i].name);
                return false;
            }
        }
        if (accelerometers[i].detect) {
            // Go through all possible I2C addresses for the current accelerometer and try to detect it
            for (u32 a = 0; a < count_of(accelerometers[i].addr); a++) {
                byte addr = accelerometers[i].addr[a];
                if (addr == NOADDR)
                    continue;
                printfbw(aahrs, "Scanning for accelerometer \"%s\" at I2C 0x%02x", accelerometers[i].name, addr);
                if (accelerometers[i].detect(addr, imu->state)) {
                    printfbw(aahrs, "Detected accelerometer \"%s\" at I2C 0x%02x", accelerometers[i].name, addr);
                    imu->acc->addr = addr;
                    detected = true;
                    break;
                }
            }
        }
        if (detected) {
            // Successfully detected an accelerometer, initialize it (create, set parameters)
            if (imu->acc->create) {
                if (!imu->acc->create(imu->acc, imu->state)) {
                    printfbw(aahrs, "ERROR: could not create accelerometer \"%s\" at I2C 0x%02x", accelerometers[i].name,
                             imu->acc->addr);
                    if (imu->acc->destroy)
                        imu->acc->destroy(imu->acc, imu->state);
                    if (imu->state)
                        free(imu->state);
                    imu->state = NULL;
                    return false;
                } else {
                    printfbw(aahrs, "Successfully created accelerometer \"%s\" at I2C 0x%02x", accelerometers[i].name,
                             imu->acc->addr);
                }
            }
            if (imu->acc->set_scale)
                imu->acc->set_scale(imu->acc, imu->state, opts->scale);
            if (imu->acc->set_odr)
                imu->acc->set_odr(imu->acc, imu->state, opts->odr);
            return true;
        } else {
            // Nothing detected, clean up state if created
            if (imu->state && accelerometers[i].create_state && accelerometers[i].destroy_state)
                imu->state = accelerometers[i].destroy_state(imu->state);
            memset(imu->acc, 0, sizeof(Accelerometer));
        }
    }
    // Nothing was detected
    imu->acc = NULL;
    return false;
}

bool fusion_accelerometer_get(IMU *imu, f32 *x, f32 *y, f32 *z) {
    if (!imu->acc || !imu->acc->read)
        return false;
    if (!imu->acc->read(imu->acc, imu->state)) {
        printfbw(aahrs, "ERROR: could not read from accelerometer");
        return false;
    }

    if (x)
        *x = (imu->acc->scale * imu->acc->ax) + imu->acc->offset_ax;
    if (y)
        *y = (imu->acc->scale * imu->acc->ay) + imu->acc->offset_ay;
    if (z)
        *z = (imu->acc->scale * imu->acc->az) + imu->acc->offset_az;
    return true;
}

bool fusion_accelerometer_set_offset(IMU *imu, f32 x, f32 y, f32 z) {
    if (!imu || !imu->acc)
        return false;

    imu->acc->offset_ax = x;
    imu->acc->offset_ay = y;
    imu->acc->offset_az = z;
    return true;
}

bool fusion_accelerometer_get_offset(IMU *imu, f32 *x, f32 *y, f32 *z) {
    if (!imu || !imu->acc)
        return false;

    if (x)
        *x = imu->acc->offset_ax;
    if (y)
        *y = imu->acc->offset_ay;
    if (z)
        *z = imu->acc->offset_az;
    return true;
}

bool fusion_accelerometer_get_scale(IMU *imu, f32 *scale) {
    if (!imu || !imu->acc || !imu->acc->get_scale || !scale)
        return false;

    return imu->acc->get_scale(imu->acc, imu->state, scale);
}

bool fusion_accelerometer_set_scale(IMU *imu, f32 scale) {
    if (!imu || !imu->acc || !imu->acc->set_scale)
        return false;

    return imu->acc->set_scale(imu->acc, imu->state, scale);
}

bool fusion_accelerometer_get_odr(IMU *imu, f32 *hertz) {
    if (!imu || !imu->acc || !imu->acc->get_odr || !hertz)
        return false;

    return imu->acc->get_odr(imu->acc, imu->state, hertz);
}

bool fusion_accelerometer_set_odr(IMU *imu, f32 hertz) {
    if (!imu || !imu->acc || !imu->acc->set_odr)
        return false;

    return imu->acc->set_odr(imu->acc, imu->state, hertz);
}
