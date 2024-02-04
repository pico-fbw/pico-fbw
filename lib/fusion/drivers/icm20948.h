#ifndef __ICM20948_H
#define __ICM20948_H

#include <stdint.h>

#include "lib/fusion/fusion.h"

#include "drivers.h"

int8_t ICM20948_init(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg);
int8_t ICM20948_read(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg);

/* I2C Addresses*/
#define ICM20948_I2C_ADDR_LOW            (0x68)
#define ICM20948_I2C_ADDR_HIGH           (0x69)

/* Expected chip WHO_AM_I value*/
#define ICM20948_WHO_AM_I_EXPECTED       (0xEA)

/* Bank selection register and selections*/
#define ICM20948_BANK_SEL                (0x7F)
#define ICM20948_SEL_BANK_0              (0x00)
#define ICM20948_SEL_BANK_1              (0x10)
#define ICM20948_SEL_BANK_2              (0x20)
#define ICM20948_SEL_BANK_3              (0x30)

/* BANK0 REGISTERS DEFINITION START*/
#define ICM20948_WHO_AM_I                (0x00)
#define ICM20948_USER_CTRL               (0x03)
#define ICM20948_LP_CONFIG               (0x05)
#define ICM20948_PWR_MGMT_1              (0x06)
#define ICM20948_PWR_MGMT_2              (0x07)

/* Interrupt configuration registers*/
#define ICM20948_INT_PIN_CFG             (0x0F)
#define ICM20948_INT_ENABLE              (0x10)
#define ICM20948_INT_ENABLE_1            (0x11)
#define ICM20948_INT_ENABLE_2            (0x12)
#define ICM20948_INT_ENABLE_3            (0x13)
#define ICM20948_I2C_MST_STATUS          (0x17)
#define ICM20948_INT_STATUS              (0x19)
#define ICM20948_INT_STATUS_1            (0x1A)
#define ICM20948_INT_STATUS_2            (0x1B)
#define ICM20948_INT_STATUS_3            (0x1C)
#define ICM20948_DELAY_TIMEH             (0x28)
#define ICM20948_DELAY_TIMEL             (0x29)

/* Accel data registers*/
#define ICM20948_ACCEL_XOUT_H            (0x2D)
#define ICM20948_ACCEL_XOUT_L            (0x2E)
#define ICM20948_ACCEL_YOUT_H            (0x2F)
#define ICM20948_ACCEL_YOUT_L            (0x30)
#define ICM20948_ACCEL_ZOUT_H            (0x31)
#define ICM20948_ACCEL_ZOUT_L            (0x32)

/* Gyro data registers*/
#define ICM20948_GYRO_XOUT_H             (0x33)
#define ICM20948_GYRO_XOUT_L             (0x34)
#define ICM20948_GYRO_YOUT_H             (0x35)
#define ICM20948_GYRO_YOUT_L             (0x36)
#define ICM20948_GYRO_ZOUT_H             (0x37)
#define ICM20948_GYRO_ZOUT_L             (0x38)

/* Temperature data registers*/
#define ICM20948_TEMP_OUT_H              (0x39)
#define ICM20948_TEMP_OUT_L              (0x3A)

/* FIFO configuration registers*/
#define ICM20948_FIFO_EN_1               (0x66)
#define ICM20948_FIFO_EN_2               (0x67)
#define ICM20948_FIFO_RST                (0x68)
#define ICM20948_FIFO_MODE               (0x69)
#define ICM20948_FIFO_COUNTH             (0x70)
#define ICM20948_FIFO_COUNTL             (0x71)
#define ICM20948_FIFO_R_W                (0x72)
#define ICM20948_DATA_RDY_STATU          (0x74)
#define ICM20948_FIFO_CFG                (0x76)

/* DMP memory access*/
#define ICM20948_MEM_START_ADDR          (0x7E)
#define ICM20948_MEM_R_W                 (0x7F)
#define ICM20948_MEM_BANK_SEL            (0x7C)

/* BANK0 REGISTERS DEFINITION END*/
/* BANK1 REGISTERS DEFINITION START*/

/* Self-test registers*/
#define ICM20948_SELF_TEST_X_GYRO        (0x02)
#define ICM20948_SELF_TEST_Y_GYRO        (0x03)
#define ICM20948_SELF_TEST_Z_GYRO        (0x04)
#define ICM20948_SELF_TEST_X_ACCEL       (0x0E)
#define ICM20948_SELF_TEST_Y_ACCEL       (0x0F)
#define ICM20948_SELF_TEST_Z_ACCEL       (0x10)

/* Accel offset cancellation registers*/
#define ICM20948_XA_OFFSET_H             (0x14)
#define ICM20948_XA_OFFSET_L             (0x15)
#define ICM20948_YA_OFFSET_H             (0x17)
#define ICM20948_YA_OFFSET_L             (0x18)
#define ICM20948_ZA_OFFSET_H             (0x1A)
#define ICM20948_ZA_OFFSET_L             (0x1B)

/* PLL clock period error*/
#define ICM20948_TIMEBASE_CORRECTION_PLL (0x28)

/* BANK1 REGISTERS DEFINITION END*/
/* BANK2 REGISTERS DEFINITION START*/

/* Gyro configuration registers*/
#define ICM20948_GYRO_SMPLRT_DIV         (0x00)
#define ICM20948_GYRO_CONFIG_1           (0x01)
#define ICM20948_GYRO_CONFIG_2           (0x02)

/* Gyro offset cancellation registers*/
#define ICM20948_XG_OFFSET_H             (0x03)
#define ICM20948_XG_OFFSET_L             (0x04)
#define ICM20948_YG_OFFSET_H             (0x05)
#define ICM20948_YG_OFFSET_L             (0x06)
#define ICM20948_ZG_OFFSET_H             (0x07)
#define ICM20948_ZG_OFFSET_L             (0x08)

/* ODR start-time alignment*/
#define ICM20948_ODR_ALIGN_EN            (0x09)

/* Accel configuration registers*/
#define ICM20948_ACCEL_SMPLRT_DIV_1      (0x10)
#define ICM20948_ACCEL_SMPLRT_DIV_2      (0x11)
#define ICM20948_ACCEL_INTEL_CTRL        (0x12)
#define ICM20948_ACCEL_WOM_THR           (0x13)
#define ICM20948_ACCEL_CONFIG_1          (0x14)
#define ICM20948_ACCEL_CONFIG_2          (0x15)

/* FSYNC event config*/
#define ICM20948_FSYNC_CONFIG            (0x52)

/* Temp configuration registers*/
#define ICM20948_TEMP_CONFIG             (0x53)

/* DMP in ACC DPLF mode*/
#define ICM20948_MOD_CTRL_USR            (0x54)

/* BANK2 REGISTERS DEFINITION END*/

#endif // __ICM20948_H
