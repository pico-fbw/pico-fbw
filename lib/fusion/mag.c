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

MagnetometerDetails magnetometers[] = {
    {"ICM20948",
     {0x0C, NOADDR},
     {.create = icm20948_mag_create,
      .read = icm20948_mag_read,
      .get_odr = icm20948_mag_get_odr,
      .set_odr = icm20948_mag_set_odr},
     icm20948_mag_detect,
     icm20948_state_create,
     icm20948_state_destroy},
};

bool fusion_magnetometer_find(IMU *imu, const MagnetometerOptions *opts) {
    if (!imu)
        return false;

    for (u32 i = 0; i < count_of(magnetometers); i++) {
        imu->mag = &(magnetometers[i].device);
        bool detected = false;
        if (!imu->state && magnetometers[i].create_state) {
            imu->state = magnetometers[i].create_state();
            if (!imu->state) {
                printfbw(aahrs, "ERROR: could not create user data for magnetometer \"%s\"", magnetometers[i].name);
                return false;
            }
        }
        if (magnetometers[i].detect) {
            for (u32 a = 0; a < count_of(magnetometers[i].addr); a++) {
                byte addr = magnetometers[i].addr[a];
                if (addr == NOADDR)
                    continue;
                printfbw(aahrs, "Scanning for magnetometer \"%s\" at I2C 0x%02x", magnetometers[i].name, addr);
                if (magnetometers[i].detect(addr, imu->state)) {
                    printfbw(aahrs, "Detected magnetometer \"%s\" at I2C 0x%02x", magnetometers[i].name, addr);
                    imu->mag->addr = addr;
                    detected = true;
                    break;
                }
            }
        }
        if (detected) {
            imu->mag->opts = *opts;
            if (imu->mag->create) {
                if (!imu->mag->create(imu->mag, imu->state)) {
                    printfbw(aahrs, "ERROR: could not create magnetometer \"%s\" at I2C 0x%02x", magnetometers[i].name,
                             imu->mag->addr);
                    if (imu->mag->destroy)
                        imu->mag->destroy(imu->mag, imu->state);
                    if (imu->state)
                        free(imu->state);
                    imu->state = NULL;
                    return false;
                } else {
                    printfbw(aahrs, "Successfully created magnetometer \"%s\" at I2C 0x%02x", magnetometers[i].name,
                             imu->mag->addr);
                }
            }
            if (imu->mag->set_scale)
                imu->mag->set_scale(imu->mag, imu->state, opts->scale);
            if (imu->mag->set_odr)
                imu->mag->set_odr(imu->mag, imu->state, opts->odr);
            imu->mag->orientation[0] = 1.f;
            imu->mag->orientation[1] = 0.f;
            imu->mag->orientation[2] = 0.f;
            imu->mag->orientation[3] = 0.f;
            imu->mag->orientation[4] = 1.f;
            imu->mag->orientation[5] = 0.f;
            imu->mag->orientation[6] = 0.f;
            imu->mag->orientation[7] = 0.f;
            imu->mag->orientation[8] = 1.f;
            return true;
        } else {
            // Nothing detected, clean up state if created
            if (imu->state && magnetometers[i].create_state && magnetometers[i].destroy_state)
                imu->state = magnetometers[i].destroy_state(imu->state);
            memset(imu->mag, 0, sizeof(Magnetometer));
        }
    }
    // Nothing was detected
    imu->mag = NULL;
    return false;
}

bool fusion_magnetometer_get(IMU *imu, f32 *x, f32 *y, f32 *z) {
    f32 mxb, myb, mzb;

    if (!imu->mag || !imu->mag->read)
        return false;
    if (!imu->mag->read(imu->mag, imu->state)) {
        printfbw(aahrs, "ERROR: could not read from magnetometer");
        return false;
    }

    // LOG(LL_DEBUG, ("Raw: mx=%d my=%d mz=%d", imu->mag->mx, imu->mag->my, imu->mag->mz));
    mxb = imu->mag->bias[0] * imu->mag->mx * imu->mag->scale;
    myb = imu->mag->bias[1] * imu->mag->my * imu->mag->scale;
    mzb = imu->mag->bias[2] * imu->mag->mz * imu->mag->scale;
    if (x)
        *x = (mxb * imu->mag->orientation[0] + myb * imu->mag->orientation[1] + mzb * imu->mag->orientation[2]);
    if (y)
        *y = (mxb * imu->mag->orientation[3] + myb * imu->mag->orientation[4] + mzb * imu->mag->orientation[5]);
    if (z)
        *z = (mxb * imu->mag->orientation[6] + myb * imu->mag->orientation[7] + mzb * imu->mag->orientation[8]);
    return true;
}

bool fusion_magnetometer_get_orientation(IMU *imu, f32 v[9]) {
    if (!imu || !imu->mag || !v)
        return false;

    memcpy(v, imu->mag->orientation, sizeof(f32) * 9);
    return true;
}

bool fusion_magnetometer_set_orientation(IMU *imu, f32 v[9]) {
    if (!imu || !imu->mag || !v)
        return false;

    memcpy(imu->mag->orientation, v, sizeof(f32) * 9);
    return true;
}

bool fusion_magnetometer_get_scale(IMU *imu, f32 *scale) {
    if (!imu || !imu->mag || !imu->mag->get_scale || !scale)
        return false;

    return imu->mag->get_scale(imu->mag, imu->state, scale);
}

bool fusion_magnetometer_set_scale(IMU *imu, f32 scale) {
    if (!imu || !imu->mag || !imu->mag->set_scale)
        return false;

    return imu->mag->set_scale(imu->mag, imu->state, scale);
}

bool fusion_magnetometer_get_odr(IMU *imu, f32 *hertz) {
    if (!imu || !imu->mag || !imu->mag->get_odr || !hertz)
        return false;

    return imu->mag->get_odr(imu->mag, imu->state, hertz);
}

bool fusion_magnetometer_set_odr(IMU *imu, f32 hertz) {
    if (!imu || !imu->mag || !imu->mag->set_odr)
        return false;

    return imu->mag->set_odr(imu->mag, imu->state, hertz);
}
