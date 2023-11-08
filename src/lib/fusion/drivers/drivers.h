#ifndef __DRIVERS_H
#define __DRIVERS_H

#include <stdint.h>

#include "hardware/i2c.h"

#define DRIVER_I2C i2c0 // The I2C bus to use for communicating with sensors
#define DRIVER_TIMEOUT_US 35000 // The maximum time in microseconds that the driver will wait for a response from the sensor

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
    uint16_t writeTo; /* Address where the value is writes to.*/
    uint8_t value;    /* value. Note that value should be shifted based on the bit position.*/
    uint8_t mask;     /* mask of the field to be set with given value.*/
} registerwritelist_t;

/*!
 * @brief This structure defines the Read command List.
 */
typedef struct
{
    uint16_t readFrom; /* Address where the value is read from .*/
    uint8_t numBytes;  /* Number of bytes to read.*/
} registerReadlist_t;

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
    uint8_t deviceInstance;
} registerDeviceInfo_t;

void driver_init();
int32_t driver_read(registerDeviceInfo_t *devInfo, uint16_t peripheralAddress, const registerReadlist_t *pReadList, uint8_t *pOutBuf);
int32_t driver_read_register(registerDeviceInfo_t *devInfo, uint16_t peripheralAddress, uint8_t offset, uint8_t len, uint8_t *pOutBuf);
int8_t driver_write_list(registerDeviceInfo_t *devInfo, uint16_t peripheralAddress, const registerwritelist_t *pRegWriteList);

#endif //__DRIVERS_H
