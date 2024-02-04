#ifndef __AK09916_H
#define __AK09916_H

#include <stdint.h>

#include "lib/fusion/fusion.h"

#include "drivers.h"

int8_t AK09916_init(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg);
int8_t AK09916_read(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg);

/* I2C Addresses*/
#define AK09916_I2C_ADDR                (0x0C)

/* Expected chip DEVICE_ID value*/
#define AK09916_DEVICE_ID_EXPECTED      (0x09)

/* Device control registers #1*/
#define AK09916_DEVICE_ID               (0x01)
#define AK09916_STATUS_1                (0x10)

/* Mag data registers*/
#define AK09916_MAG_DATA_X_L            (0x11)
#define AK09916_MAG_DATA_X_H            (0x12)
#define AK09916_MAG_DATA_Y_L            (0x13)
#define AK09916_MAG_DATA_Y_H            (0x14)
#define AK09916_MAG_DATA_Z_L            (0x15)
#define AK09916_MAG_DATA_Z_H            (0x16)

/* Device control registers #2*/
#define AK09916_STATUS_2                (0x18)
#define AK09916_CONTROL_2               (0x31)
#define AK09916_CONTROL_3               (0x32)

#endif // __AK09916_H
