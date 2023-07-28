/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "../modes/modes.h"
#include "../config.h"

#include "imu.h"

/**
 * Writes a byte value directly to the IMU.
 * @param address The address to write to.
 * @param value The value to write.
 * @return Number of bytes written, or PICO_ERROR_GENERIC if address not acknowledged, no device present.
*/
static inline int imu_write(uint8_t address, uint8_t value) {
    uint8_t cmd[2] = {address, value};
    return i2c_write_blocking(IMU_I2C, CHIP_REGISTER, cmd, 2, true);
}

#if defined(IMU_BNO055)

    /**
     * Changes the working mode of the IMU (currently only used for BNO055).
     * @param mode The code of the mode to change into (for example, 0x0C for NDOF).
     * @return true if success, false if failure.
    */
    static bool imu_changeMode(uint8_t mode) {
        FBW_DEBUG_printf("[imu] changing to mode 0x%02X\n", mode);
        imu_write(OPR_MODE_REGISTER, mode);
        sleep_ms(100);
        // Check to ensure mode has changed properly by reading it back
        uint8_t currentMode;
        i2c_write_blocking(IMU_I2C, CHIP_REGISTER, &OPR_MODE_REGISTER, 1, true);
        i2c_read_blocking(IMU_I2C, CHIP_REGISTER, &currentMode, 1, false);
        if (currentMode == mode) {
            return true;
        } else {
            return false;
        }
    }

#elif defined(IMU_MPU6050)

    // TODO: implement MPU6050 fusion
    // https://github.com/jrowberg/i2cdevlib/tree/master/RP2040/MPU6050

#endif

int imu_init() {
    // Steps to init are the same for all IMUs
    FBW_DEBUG_printf("[imu] initializing ");
    if (IMU_I2C == i2c0) {
        FBW_DEBUG_printf("i2c0\n");
    } else if (IMU_I2C == i2c1) {
        FBW_DEBUG_printf("i2c1\n");
    } else {
        FBW_DEBUG_printf("\n");
    }
    i2c_init(IMU_I2C, CHIP_FREQ_KHZ * 1000);
    gpio_set_function(IMU_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(IMU_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(IMU_SDA_PIN);
    gpio_pull_up(IMU_SCL_PIN);
    // Query the ID register for expected values to confirm identity/check comms
    FBW_DEBUG_printf("[imu] searching for ");
    #if defined(IMU_BNO055)
        FBW_DEBUG_printf("BNO055\n");
    #elif defined(IMU_MPU6050)
        FBW_DEBUG_printf("MPU6050\n");
    #else
        #error No IMU module was defined.
    #endif
    IMU_DEBUG_printf("[imu] checking ID (writing 0x%02X [ID_REGISTER] to 0x%02X [CHIP_REGISTER]) with timeout of %dus...\n", ID_REGISTER, CHIP_REGISTER, IMU_TIMEOUT_US);
    int result = i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &ID_REGISTER, 1, true, IMU_TIMEOUT_US);
    if (result == PICO_ERROR_GENERIC) {
        IMU_DEBUG_printf("[imu] ERROR: address not acknowledged (no device present?)\n");
        return PICO_ERROR_GENERIC;
    } else if (result == PICO_ERROR_TIMEOUT) {
        IMU_DEBUG_printf("[imu] ERROR: write timeout occured\n");
        return PICO_ERROR_TIMEOUT;
    } else if (result == 1) { // Correct number of bytes written should be 1
        IMU_DEBUG_printf("[imu] address acknowledged, attempting to read ID...\n");
        uint8_t id;
        result = i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, &id, 1, false, IMU_TIMEOUT_US);
        if (result == PICO_ERROR_GENERIC) {
            IMU_DEBUG_printf("[imu] ERROR: address not acknowledged\n");
            return PICO_ERROR_GENERIC;
        } else if (result== PICO_ERROR_TIMEOUT) {
            IMU_DEBUG_printf("[imu] ERROR: write timeout occured\n");
            return PICO_ERROR_TIMEOUT;
        } else {
            IMU_DEBUG_printf("[imu] ID: 0x%02X\n", id);
            if (id == CHIP_ID) {
                IMU_DEBUG_printf("[imu] ID matches expected value\n");
                return 0;
            } else {
                IMU_DEBUG_printf("[imu] ERROR: ID does not match expected value\n");
                return 1;
            }
        }
    } else {
        IMU_DEBUG_printf("[imu] ERROR: unknown error occured\n");
        return PICO_ERROR_GENERIC;
    }
}

void imu_deinit() {
    i2c_deinit(IMU_I2C);
}

bool imu_configure() {
    FBW_DEBUG_printf("[imu] configuring...\n");
    #if defined(IMU_BNO055)
        imu_write(SYS_REGISTER, 0x40);
        imu_write(SYS_REGISTER, 0x01);
        imu_write(PWR_MODE_REGISTER, PWR_MODE_NORMAL);
        sleep_ms(50);
        imu_write(AXIS_MAP_CONF_REGISTER, 0x24);
        imu_write(AXIS_MAP_SIGN_REGISTER, 0x00);
        imu_write(0x3B, 0b00000001);
        sleep_ms(30);
        return imu_changeMode(MODE_NDOF);
    #elif defined(IMU_MPU6050)
        imu_write(PWR_MODE_REGISTER, 0x80);
        sleep_ms(50);
        imu_write(PWR_MODE_REGISTER, 0x00);
        imu_write(SMPLRT_DIV_REGISTER, 0x00);
        imu_write(CONFIG_REGISTER, 0x00);
        imu_write(GYRO_CONFIG_REGISTER, 0x00);
        imu_write(ACCEL_CONFIG_REGISTER, 0x08);
    #endif
}

inertialAngles imu_getAngles() {
    // Get angle data from IMU
    uint8_t gyro_data[6];
    int timeout0 = i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &GYRO_BEGIN_REGISTER, 1, true, IMU_TIMEOUT_US);
    int timeout1 = i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, gyro_data, 6, false, IMU_TIMEOUT_US);
    // Check if any I2C related errors occured, if so, set IMU as unsafe and return no data
    if (timeout0 == PICO_ERROR_GENERIC || timeout0 == PICO_ERROR_TIMEOUT || timeout1 == PICO_ERROR_GENERIC || timeout1 == PICO_ERROR_TIMEOUT) {
        setIMUSafe(false);
        return (inertialAngles){0};
    }
    // Bit shifting and data conversions vary by type
    #if defined(IMU_BNO055)
        // Perform the necessary bit shifts to combine the high byte and low byte into one signed integer
        int16_t rawX = (gyro_data[1] << 8) | gyro_data[0]; // (heading)
        int16_t rawY = (gyro_data[3] << 8) | gyro_data[2]; // (roll)
        int16_t rawZ = (gyro_data[5] << 8) | gyro_data[4]; // (pitch)

        // Convert raw data into angles
        float x = (float)(rawX / 16.0);
        float y = (float)(rawY / 16.0);
        float z = (float)(rawZ / 16.0);
    #elif defined(IMU_MPU6050)
        int16_t rawX = (gyro_data[0] << 8) | gyro_data[1]; // (x)
        int16_t rawY = (gyro_data[2] << 8) | gyro_data[3]; // (y)
        int16_t rawZ = (gyro_data[4] << 8) | gyro_data[5]; // (z)

        float x = (float)((rawX / 131.0));
        float y = (float)((rawY / 131.0));
        float z = (float)((rawZ / 131.0));
    #endif
    // Map IMU axes to aircraft axes
    float roll, pitch, heading = -100.0f;
    switch (IMU_X_AXIS) {
        case ROLL_AXIS:
            roll = x;
            break;
        case PITCH_AXIS:
            pitch = x;
            break;
        case YAW_AXIS:
            heading = x;
            break;
        default:
            // This should never happen, handled in preprocessor
            break;
    }
    switch (IMU_Y_AXIS) {
        case ROLL_AXIS:
            roll = y;
            break;
        case PITCH_AXIS:
            pitch = y;
            break;
        case YAW_AXIS:
            heading = y;
            break;
        default:
            break;
    }
    switch (IMU_Z_AXIS) {
        case ROLL_AXIS:
            roll = z;
            break;
        case PITCH_AXIS:
            pitch = z;
            break;
        case YAW_AXIS:
            heading = z;
            break;
        default:
            break;
    }
    // Derive yaw from heading
    float yaw_degrees = heading;
    if (yaw_degrees >= 180) {
        yaw_degrees -= 360;
    }
    // Check for writing errors
    if (heading == -100.0f || roll == -100.0f || pitch == -100.0f) {
        setIMUSafe(false);
        return (inertialAngles){0};
    }
    // Compose into data struct and return
    return (inertialAngles){heading, roll, pitch, yaw_degrees};
}

inertialAccel imu_getAccel() {
    // Same process as getting Euler data
    uint8_t accel_data[6];
    int timeout0 = i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &ACCEL_BEGIN_REGISTER, 1, true, IMU_TIMEOUT_US);
    int timeout1 = i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, accel_data, 6, false, IMU_TIMEOUT_US);
    if (timeout0 == PICO_ERROR_GENERIC || timeout0 == PICO_ERROR_TIMEOUT || timeout1 == PICO_ERROR_GENERIC || timeout1 == PICO_ERROR_TIMEOUT) {
        setIMUSafe(false);
        return (inertialAccel){0};
    }
    #if defined(IMU_BNO055)
        int16_t rawX = (accel_data[1] << 8) | accel_data[0];
        int16_t rawY = (accel_data[3] << 8) | accel_data[2];
        int16_t rawZ = (accel_data[5] << 8) | accel_data[4];

        float x = (float)(rawX / 100.00);
        float y = (float)(rawY / 100.00);
        float z = (float)(rawZ / 100.00);
    #elif defined(IMU_MPU6050)
        int16_t rawX = (accel_data[0] << 8) | accel_data[1];
        int16_t rawY = (accel_data[2] << 8) | accel_data[3];
        int16_t rawZ = (accel_data[4] << 8) | accel_data[5];

        float x = (float)(rawX / 8192.0);
        float y = (float)(rawY / 8192.0);
        float z = (float)(rawZ / 8192.0);
    #endif
    return (inertialAccel){x, y, z};
}
