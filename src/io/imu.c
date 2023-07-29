/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Markus Hintersteiner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * **/

/**
 * Huge thanks to the contributors of the 'i2cdevlib' repository for the amazing work on the MPU6050 fusion algorithm!
 * Check it out here: https://github.com/jrowberg/i2cdevlib
*/

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

#ifdef IMU_MPU6050
    #include <math.h>
    #include "../lib/dmp.h"
#endif

#include "imu.h"

// Holds persistant Euler data between calls to keep a constant stream of data whether the IMU is ready or not.
typedef struct Euler {
    float x;
    float y;
    float z;
} Euler;
static Euler euler;

/**
 * Writes a byte value directly to the IMU.
 * @param address The address to write to.
 * @param value The value to write.
 * @return Number of bytes written, or PICO_ERROR_GENERIC if address not acknowledged, no device present.
*/
static inline int imu_write(uint8_t address, uint8_t value) {
    uint8_t cmd[2] = {address, value};
    return i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, cmd, 2, true, IMU_TIMEOUT_US);
}

#if defined(IMU_BNO055)

    /**
     * Changes the working mode of the BNO055.
     * @param mode The code of the mode to change into (for example, 0x0C for NDOF).
     * @return true if success, false if failure.
    */
    static bool bno_changeMode(uint8_t mode) {
        FBW_DEBUG_printf("[imu] changing to mode 0x%02X\n", mode);
        imu_write(OPR_MODE_REGISTER, mode);
        sleep_ms(100);
        // Check to ensure mode has changed properly by reading it back
        uint8_t currentMode;
        i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &OPR_MODE_REGISTER, 1, true, IMU_TIMEOUT_US);
        i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, &currentMode, 1, false, IMU_TIMEOUT_US);
        if (currentMode == mode) {
            return true;
        } else {
            return false;
        }
    }

#elif defined(IMU_MPU6050)

    static uint16_t fifoCount;
    static uint8_t fifoBuf[DMP_PACKET_SIZE];

    /**
     * Gets the current count of the MPU's FIFO.
     * @return The count of the FIFO.
    */
    static uint16_t mpu_getFIFOCount() {
        uint8_t buf[2];
        i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &DMP_RA_FIFO_COUNT, 1, true, IMU_TIMEOUT_US);
        i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, buf, 2, false, IMU_TIMEOUT_US);
        return (((uint16_t)buf[0]) << 8) | buf[1];
    }

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
        return bno_changeMode(MODE_NDOF);
    #elif defined(IMU_MPU6050)
        imu_write(PWR_MODE_REGISTER, 0x80); // Reset power
        sleep_ms(100);
        imu_write(USER_CONTROL_REGISTER, 0b111); // Reset again
        sleep_ms(100);
        imu_write(PWR_MODE_REGISTER, 0x01); // PLL_X_GYRO is a slightly better clock source
        imu_write(INTERRUPTS_ENABLED_REGISTER, 0x00); // Disable interrupts
        imu_write(FIFO_ENABLED_REGISTER, 0x00); // Disable FIFO (using DMP FIFO)
        imu_write(SMPLRT_DIV_REGISTER, 0x04); // Sample rate = gyro rate / (1 + SMPLRT_DIV)
        imu_write(CONFIG_REGISTER, 0x01); // Digital Low Pass Filter (DLPF) @ 188Hz
        imu_write(ACCEL_CONFIG_REGISTER, 0x00); // Accel full scale = 2g
        imu_write(GYRO_CONFIG_REGISTER, 0x18); // Gyro full scale = +2000 deg/s
        IMU_DEBUG_printf("[imu] writing DMP to memory...\n");
        dmp_writeMemoryBlock(dmp, DMP_SIZE, 0, 0); // Write DMP to memory
        uint8_t cmd[3] = {DMP_PROG_START_ADDR, 0x04, 0x00}; // DMP program start address
        i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, cmd, 3, true, IMU_TIMEOUT_US);
        imu_write(USER_CONTROL_REGISTER, 0xC2); // Enable and reset DMP FIFO
        imu_write(INTERRUPTS_ENABLED_REGISTER, 0x02); // Enable RAW_DMP_INT

        // TODO: port accel and gyro calibrations from DMP lib/i2cdevlib
    #endif
}

inertialAngles imu_getAngles() {
    #if defined(IMU_BNO055)

        // Get angle data from IMU
        uint8_t euler_data[6];
        int timeout0 = i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &EULER_BEGIN_REGISTER, 1, true, IMU_TIMEOUT_US);
        int timeout1 = i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, euler_data, 6, false, IMU_TIMEOUT_US);
        // Check if any I2C related errors occured, if so, set IMU as unsafe and return no data
        if (timeout0 == PICO_ERROR_GENERIC || timeout0 == PICO_ERROR_TIMEOUT || timeout1 == PICO_ERROR_GENERIC || timeout1 == PICO_ERROR_TIMEOUT) {
            setIMUSafe(false);
            return (inertialAngles){0};
        }

        // Bit shift to combine the high byte and low byte into one signed integer
        int16_t raw[3];
        for (int i = 0; i < 3; i++) {
            raw[i] = (euler_data[i * 2 + 1] << 8) | euler_data[i * 2];
        }
        // Convert raw data into Euler angles
        euler.x = (float)(raw[0] / 16.0);
        euler.y = (float)(raw[1] / 16.0);
        euler.z = (float)(raw[2] / 16.0);

    #elif defined(IMU_MPU6050)

        // Get current FIFO count
        fifoCount = mpu_getFIFOCount();
        if (fifoCount >= 1024) {
            // FIFO overflow, reset
            imu_write(USER_CONTROL_REGISTER, 0b00000010);
        } else {
            if (fifoCount >= DMP_PACKET_SIZE) {
                // FIFO is full, read out data
                i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &DMP_RA_FIFO_R_W, 1, true, IMU_TIMEOUT_US);
                i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, fifoBuf, DMP_PACKET_SIZE, false, IMU_TIMEOUT_US);
                fifoCount -= DMP_PACKET_SIZE;
                // Obtain quaternion data from FIFO
                float q[4];
                for (uint8_t i = 0; i < 4; i++) {
                    int16_t qI = ((fifoBuf[i * 4] << 8) | fifoBuf[i * 4 + 1]);
                    q[i] = (float)qI / 16384.0f;
                }
                // Convert quaternion to Euler angles
                euler.x = (atan2(2*q[1]*q[2] - 2*q[0]*q[3], 2*q[0]*q[0] + 2*q[1]*q[1] - 1)) * (180 / M_PI);
                euler.y = (-asin(2*q[1]*q[3] + 2*q[0]*q[2])) * (180 / M_PI);
                euler.z = (atan2(2*q[2]*q[3] - 2*q[0]*q[1], 2*q[0]*q[0] + 2*q[3]*q[3] - 1)) * (180 / M_PI);
            }
        }

    #endif
    // Map IMU axes to aircraft axes
    float roll, pitch, heading = -100.0f;
    switch (IMU_X_AXIS) {
        case ROLL_AXIS:
            roll = euler.x;
            break;
        case PITCH_AXIS:
            pitch = euler.x;
            break;
        case YAW_AXIS:
            heading = euler.x;
            break;
        default:
            // This should never happen, handled in preprocessor
            break;
    }
    switch (IMU_Y_AXIS) {
        case ROLL_AXIS:
            roll = euler.y;
            break;
        case PITCH_AXIS:
            pitch = euler.y;
            break;
        case YAW_AXIS:
            heading = euler.y;
            break;
        default:
            break;
    }
    switch (IMU_Z_AXIS) {
        case ROLL_AXIS:
            roll = euler.z;
            break;
        case PITCH_AXIS:
            pitch = euler.z;
            break;
        case YAW_AXIS:
            heading = euler.z;
            break;
        default:
            break;
    }
    // Derive yaw from heading
    float yaw_degrees = heading;
    if (yaw_degrees >= 180) {
        yaw_degrees -= 360;
    }
    // Check for errors
    if (heading == -100.0f || roll == -100.0f || pitch == -100.0f) {
        setIMUSafe(false);
        return (inertialAngles){0};
    }
    // Compose into data struct and return
    return (inertialAngles){heading, roll, pitch, yaw_degrees};
}
