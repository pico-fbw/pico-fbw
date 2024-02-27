#pragma once

#include "platform/int.h"

#define DRIVER_FREQ_KHZ 400  // The frequency of the I2C bus in kHz
#define DRIVER_INIT_ATTEMPTS 5 // The maximum number of attempts that the driver will make to initialize the sensor

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* @brief This enum defines Write flag for the Register Write. */
typedef enum EWriteFlags
{
    WRITE_OVERWRITE = 0, /* Overwrite the Register Value.*/
    WRITE_MASK = 1       /* Read and Mask and OR it with Register content.*/
} EWriteFlags_t;

/* @brief This enum defines Sensor State. */
enum ESensorErrors
{
    SENSOR_ERROR_NONE = 0,
    SENSOR_ERROR_INVALID_PARAM,
    SENSOR_ERROR_BAD_ADDRESS,
    SENSOR_ERROR_INIT,
    SENSOR_ERROR_WRITE,
    SENSOR_ERROR_READ,
};

/* The MAXIMUM number of Sensor Registers possible. */
#define SENSOR_MAX_REGISTER_COUNT 128 /* As per 7-Bit address. */

/* Used with the RegisterWriteList types as a list terminator */
#define __END_WRITE_DATA__            \
    {                                 \
        .writeTo = 0xFFFF, .value = 0 \
    }

/* Used with the RegisterReadList types as a list terminator */
#define __END_READ_DATA__                 \
    {                                     \
        .readFrom = 0xFFFF, .numBytes = 0 \
    }

/* Used with the Sensor Command List types as a list terminator */
#define __END_WRITE_CMD__                \
    {                                    \
        .writeTo = 0xFFFF, .numBytes = 0 \
    }

/*******************************************************************************
 * Types
 ******************************************************************************/
/*!
 * @brief This structure defines the Write command List.
 */
typedef struct
{
    u16 writeTo; /* Address where the value is writes to.*/
    u8 value;    /* value. Note that value should be shifted based on the bit position.*/
    u8 mask;     /* mask of the field to be set with given value.*/
} registerWriteList_t;

/*!
 * @brief This structure defines the Read command List.
 */
typedef struct
{
    u16 readFrom; /* Address where the value is read from .*/
    u8 numBytes;  /* Number of bytes to read.*/
} registerReadList_t;

/*!
 * @brief This is the register idle function type.
 */
typedef void (*registeridlefunction_t)(void *userParam);

/*!
 * @brief This structure defines the device specific info required by register I/O.
 */
typedef struct
{
    registeridlefunction_t idleFunction;
    void *functionParam;
    u8 deviceInstance;
} registerDeviceInfo_t;

void driver_init();
i32 driver_read(registerDeviceInfo_t *devInfo, u16 peripheralAddress, const registerReadList_t *pReadList, u8 *pOutBuf);
i32 driver_read_register(registerDeviceInfo_t *devInfo, u16 peripheralAddress, u8 offset, u8 len, u8 *pOutBuf);
i8 driver_write_list(registerDeviceInfo_t *devInfo, u16 peripheralAddress, const registerWriteList_t *pRegWriteList);
