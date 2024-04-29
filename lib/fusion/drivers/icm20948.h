#pragma once

#include <stdbool.h>
#include "platform/types.h"

#include "drivers.h"

#define ICM20948_DEFAULT_I2CADDR (0x68)
#define ICM20948_DEFAULT_M_I2CADDR (0x0C)
#define ICM20948_DEVID (0xEA)
#define ICM20948_DEVID_M (0x09)

// accelerometer/gyroscope registers
#define ICM20948_REG0_WHO_AM_I (0x00)
#define ICM20948_REG0_USER_CTRL (0x03)
#define ICM20948_REG0_LP_CONFIG (0x05)
#define ICM20948_REG0_PWR_MGMT_1 (0x06)
#define ICM20948_REG0_PWR_MGMT_2 (0x07)
#define ICM20948_REG0_INT_PIN_CFG (0x0f)
#define ICM20948_REG0_ACCEL_XOUT_H (0x2d)
#define ICM20948_REG0_GYRO_XOUT_H (0x33)
#define ICM20948_REG0_EXT_SLV_SENS_DATA_00 (0x3b)
#define ICM20948_REG0_BANK_SEL (0x7f)
#define ICM20948_REG2_GYRO_SMPLRT_DIV (0x00)
#define ICM20948_REG2_GYRO_CONFIG_1 (0x01)
#define ICM20948_REG2_ACCEL_SMPLRT_DIV_1 (0x10)
#define ICM20948_REG2_ACCEL_SMPLRT_DIV_2 (0x11)
#define ICM20948_REG2_ACCEL_CONFIG (0x14)
#define ICM20948_REG3_I2C_MST_CTRL (0x01)
#define ICM20948_REG3_I2C_SLV0_ADDR (0x03)
#define ICM20948_REG3_I2C_SLV0_REG (0x04)
#define ICM20948_REG3_I2C_SLV0_CTRL (0x05)
#define ICM20948_REG3_I2C_SLV0_DO (0x06)

// magnetometer registers (AK09916)
#define ICM20948_WHO_AM_I_M (0x01)
#define ICM20948_HXL_M (0x11)
#define ICM20948_ST2_M (0x18)
#define ICM20948_CNTL2_M (0x31)
#define ICM20948_CNTL3_M (0x32)

typedef struct ICM20948State {
    bool accgyro_initialized;
    i8 current_bank_no;
} ICM20948State;

void *icm20948_state_create();
void *icm20948_state_destroy(void *state);

bool icm20948_acc_detect(byte addr, void *state);
bool icm20948_acc_create(Accelerometer *dev, void *state);
bool icm20948_acc_read(Accelerometer *dev, void *state);
bool icm20948_acc_get_scale(Accelerometer *dev, void *state, f32 *scale);
bool icm20948_acc_set_scale(Accelerometer *dev, void *state, f32 scale);
bool icm20948_acc_get_odr(Accelerometer *dev, void *state, f32 *odr);
bool icm20948_acc_set_odr(Accelerometer *dev, void *state, f32 odr);

bool icm20948_gyro_detect(byte addr, void *state);
bool icm20948_gyro_create(Gyroscope *dev, void *state);
bool icm20948_gyro_read(Gyroscope *dev, void *state);
bool icm20948_gyro_get_scale(Gyroscope *dev, void *state, f32 *scale);
bool icm20948_gyro_set_scale(Gyroscope *dev, void *state, f32 scale);
bool icm20948_gyro_get_odr(Gyroscope *dev, void *state, f32 *odr);
bool icm20948_gyro_set_odr(Gyroscope *dev, void *state, f32 odr);

bool icm20948_mag_detect(byte addr, void *state);
bool icm20948_mag_create(Magnetometer *dev, void *state);
bool icm20948_mag_read(Magnetometer *dev, void *state);
bool icm20948_mag_get_odr(Magnetometer *dev, void *state, f32 *odr);
bool icm20948_mag_set_odr(Magnetometer *dev, void *state, f32 odr);
