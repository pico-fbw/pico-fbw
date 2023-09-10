/**
 * This file utilizes code under the MIT License. See "LICENSE" for details.
*/

/**
 * Huge thanks to the contributors of the 'i2cdevlib' repository for the amazing work on the MPU6050 fusion algorithm!
 * Check it out here: https://github.com/jrowberg/i2cdevlib
*/

/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "pico/types.h"

#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "../modes/modes.h"
#include "../config.h"

#include "../lib/dmp.h"

#include "error.h"
#include "flash.h"
#include "platform.h"

#include "imu.h"

typedef enum IMUModel {
    IMU_MODEL_UNKNOWN,
    IMU_MODEL_BNO055,
    IMU_MODEL_MPU6050
} IMUModel;

typedef enum IMUCalibrationState {
    CALIBRATION_STATE_ROLL,
    CALIBRATION_STATE_PITCH,
    CALIBRATION_STATE_YAW,
    CALIBRATION_STATE_COMPLETE
} IMUCalibrationState;

typedef enum IMUAxis {
    IMU_AXIS_NONE,
    IMU_AXIS_ROLL,
    IMU_AXIS_PITCH,
    IMU_AXIS_YAW
} IMUAxis;

typedef enum EulerAxis {
    EULER_AXIS_NONE,
    EULER_AXIS_X,
    EULER_AXIS_Y,
    EULER_AXIS_Z
} EulerAxis;

static Euler euler; // Holds persistant Euler data between calls to keep a constant stream of data, whether the IMU is ready or not

// These are all populated when imu_init() is called
// We must determine the model and some register addresses at runtime to accomodate the fbw board
static IMUModel imuModel = IMU_MODEL_UNKNOWN;
static uint CHIP_FREQ_KHZ;
static unsigned char CHIP_REGISTER;
static unsigned char ID_REGISTER;
static unsigned char CHIP_ID;
static unsigned char PWR_MODE_REGISTER;

/**
 * Writes a byte value directly to the IMU.
 * @param addr The address to write to.
 * @param val The value to write.
 * @return Number of bytes written, or PICO_ERROR_GENERIC if address not acknowledged, no device present.
 * PICO_ERROR_TIMEOUT if a timeout occured.
*/
static inline int imu_write(unsigned char addr, unsigned char val) {
    unsigned char c[2] = {addr, val};
    return i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, c, sizeof(c), true, IMU_TIMEOUT_US);
}

static inline IMUAxis getCalibrationAxis(EulerAxis axis) { return (IMUAxis)(flash_read(FLASH_SECTOR_IMU_MAP, (uint)axis)); }
static inline bool shouldCompensateAxis(EulerAxis axis) { return (bool)(flash_read(FLASH_SECTOR_IMU_MAP, (uint)(axis + 3))); }

/**
 * Changes the working mode of the BNO055.
 * @param mode The code of the mode to change into (for example, 0x0C for NDOF).
 * @return true if success, false if failure.
*/
static bool bno_changeMode(unsigned char mode) {
    IMU_DEBUG_printf("[imu] changing to mode 0x%02X\n", mode);
    imu_write(OPR_MODE_REGISTER, mode);
    sleep_ms(100);
    // Check to ensure mode has changed properly by reading it back
    unsigned char currentMode;
    i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &OPR_MODE_REGISTER, sizeof(OPR_MODE_REGISTER), true, IMU_TIMEOUT_US);
    i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, &currentMode, sizeof(currentMode), false, IMU_TIMEOUT_US);
    if (currentMode == mode) {
        return true;
    } else {
        IMU_DEBUG_printf("[imu] failed to change mode, mode is still 0x%02X, supposed to be 0x%02X\n", currentMode, mode);
        return false;
    }
}

/**
 * Loads the sensor calibration status data from the BNO055.
 * @param sys Pointer to a variable to store the system calibration status.
 * @param gyr Pointer to a variable to store the gyroscope calibration status.
 * @param acc Pointer to a variable to store the accelerometer calibration status.
 * @param mag Pointer to a variable to store the magnetometer calibration status.
 * @return Number of bytes read, or PICO_ERROR_GENERIC if address not acknowledged, no device present.
 * PICO_ERROR_TIMEOUT if a timeout occured.
*/
static int bno_getCalibrationStatus(unsigned char *sys, unsigned char *gyr, unsigned char *acc, unsigned char *mag) {
    unsigned char status;
    i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &CALIBRATION_REGISTER, sizeof(CALIBRATION_REGISTER), true, IMU_TIMEOUT_US);
    int timeout = i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, &status, sizeof(status), false, IMU_TIMEOUT_US);
    if (timeout != 1) return timeout;
    *sys = (status >> 6) & 0x03;
    *gyr = (status >> 4) & 0x03;
    *acc = (status >> 2) & 0x03;
    *mag = status & 0x03;
}

/**
 * Loads the sensor calibration data from the BNO055.
 * @param data Pointer to an array (at least 11 elements) to store the calibration data.
 * @return Number of bytes read, or PICO_ERROR_GENERIC if address not acknowledged, no device present.
 * PICO_ERROR_TIMEOUT if a timeout occured.
 * @note You must be in CONFIG mode to read this data.
*/
static int bno_getCalibrationData(int16_t *data) {
    unsigned char buf[22];
    i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &ACCEL_OFFSET_X_BEGIN_REGISTER, sizeof(ACCEL_OFFSET_X_BEGIN_REGISTER), true, IMU_TIMEOUT_US);
    int timeout = i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, buf, sizeof(buf), false, IMU_TIMEOUT_US);
    for (uint8_t i = 0; i < 11; i++) {
        data[i] = (buf[i * 2 + 1] << 8) | buf[i * 2];
        IMU_DEBUG_printf("[BNO055] read 0x%04X from LSB 0x%02X, MSB 0x%02X\n", data[i], buf[i * 2 + 1], buf[i * 2]);
    }
}

/**
 * Saves sensor calibration data to the BNO055.
 * @param data Pointer to an array (at least 11 elements) holding the calibration data to store.
 * @param mag Whether to save the magnetometer calibration data or not.
 * @return Number of bytes written, or PICO_ERROR_GENERIC if address not acknowledged, no device present.
 * PICO_ERROR_TIMEOUT if a timeout occured.
 * @note You must be in CONFIG mode to write this data.
*/
static int bno_saveCalibrationData(int16_t *data, bool mag) {
    if (mag) {
        unsigned char buf[24]; // 1 addr byte, 22 data bytes, 1 null byte (not written)
        buf[0] = ACCEL_OFFSET_X_BEGIN_REGISTER;
        for (uint8_t i = 1; i < 12; i++) {
            buf[i * 2] = data[i - 1] & 0xFF; // Store LSB
            buf[i * 2 + 1] = (data[i - 1] >> 8) & 0xFF; // Store MSB
            IMU_DEBUG_printf("[BNO055] write 0x%04X to LSB 0x%02X, MSB 0x%02X\n", data[i - 1], buf[i * 2 + 1], buf[i * 2]);
        }
        return i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, buf, sizeof(buf) - 1, true, IMU_TIMEOUT_US);
    } else {
        unsigned char acc[8]; // 1 addr, 6 data, 1 null
        acc[0] = ACCEL_OFFSET_X_BEGIN_REGISTER;
        unsigned char gyr_rad[10]; // 1 addr, 8 data, 1 null
        gyr_rad[0] = GYRO_OFFSET_X_BEGIN_REGISTER;
        for (uint8_t i = 1; i < 4; i++) {
            acc[i * 2] = data[i - 1] & 0xFF;
            acc[i * 2 + 1] = (data[i - 1] >> 8) & 0xFF;
            IMU_DEBUG_printf("[BNO055] (acc) write 0x%04X to LSB 0x%02X, MSB 0x%02X\n", data[i - 1], acc[i * 2 + 1], acc[i * 2]);
        }
        for (uint8_t i = 1; i < 5; i++) {
            gyr_rad[i * 2] = data[i + 5] & 0xFF;
            gyr_rad[i * 2 + 1] = (data[i + 5] >> 8) & 0xFF;
            IMU_DEBUG_printf("[BNO055] (gyr_rad) write 0x%04X to LSB 0x%02X, MSB 0x%02X\n", data[i + 5], gyr_rad[i * 2 + 1], gyr_rad[i * 2]);
        }
        i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, acc, sizeof(acc) - 1, true, IMU_TIMEOUT_US);
        return i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, gyr_rad, sizeof(gyr_rad) - 1, true, IMU_TIMEOUT_US);
    }
}

static uint16_t fifoCount;
static unsigned char fifoBuf[DMP_PACKET_SIZE];

struct repeating_timer updateTimer;

/**
 * Gets the current count of the MPU's FIFO.
 * @return The count of the FIFO.
*/
static uint16_t mpu_getFIFOCount() {
    unsigned char buf[2];
    i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &DMP_RA_FIFO_COUNT, 1, true, IMU_TIMEOUT_US);
    i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, buf, 2, false, IMU_TIMEOUT_US);
    return (((uint16_t)buf[0]) << 8) | buf[1];
}

/**
 * Updates the MPU's DMP.
 * This must be done continually after initialization with the MPU to ensure accurate angles;
 * the angles do not update without communication!
*/
static bool mpu_update(struct repeating_timer *t) { imu_getRawAngles(); }


int imu_init() {
    // Determine which IMU to use
    if (platform_is_fbw()) {
        // The FBW platform uses the BNO055 as the IMU, so use that regardless of what is defined
        imuModel = IMU_MODEL_BNO055;
    } else {
        // Otherwise, use whatever is defined in the config
        #if defined(IMU_BNO055)
            imuModel = IMU_MODEL_BNO055;
        #elif defined(IMU_MPU6050)
            imuModel = IMU_MODEL_MPU6050;
        #endif
    }
    // Some addresses, IDs, etc overlap between models so we have to populate those now for use later
    switch (imuModel) {
        case IMU_MODEL_BNO055:
            CHIP_FREQ_KHZ = BNO_CHIP_FREQ_KHZ;
            CHIP_REGISTER = BNO_CHIP_REGISTER;
            ID_REGISTER = BNO_ID_REGISTER;
            CHIP_ID = BNO_CHIP_ID;
            PWR_MODE_REGISTER = BNO_PWR_MODE_REGISTER;
            break;
        case IMU_MODEL_MPU6050:
            CHIP_FREQ_KHZ = MPU_CHIP_FREQ_KHZ;
            CHIP_REGISTER = MPU_CHIP_REGISTER;
            ID_REGISTER = MPU_ID_REGISTER;
            CHIP_ID = MPU_CHIP_ID;
            PWR_MODE_REGISTER = MPU_PWR_MODE_REGISTER;
            break;
    }
    // Steps to init are the same for all IMUs; only things like addresses and IDs change which has been covered above
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
    switch (imuModel) {
        case IMU_MODEL_BNO055:
            FBW_DEBUG_printf("BNO055\n");
            break;
        case IMU_MODEL_MPU6050:
            FBW_DEBUG_printf("MPU6050\n");
            break;
    }
    IMU_DEBUG_printf("[imu] checking ID (writing 0x%02X [ID_REGISTER] to 0x%02X [CHIP_REGISTER]) with timeout of %dus...\n", ID_REGISTER, CHIP_REGISTER, IMU_TIMEOUT_US);
    int result = i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &ID_REGISTER, 1, true, IMU_TIMEOUT_US);
    if (result == PICO_ERROR_GENERIC) {
        IMU_DEBUG_printf("[imu] ERROR: address not acknowledged (no/wrong device present?)\n");
        return PICO_ERROR_GENERIC;
    } else if (result == PICO_ERROR_TIMEOUT) {
        IMU_DEBUG_printf("[imu] ERROR: write timeout occured (no device present?)\n");
        return PICO_ERROR_TIMEOUT;
    } else if (result == 1) { // Correct number of bytes written should be 1
        IMU_DEBUG_printf("[imu] address acknowledged, attempting to read ID...\n");
        unsigned char id;
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
    switch (imuModel) {
        case IMU_MODEL_BNO055:
            /* imu_write(SYS_REGISTER, 0x20); // Reset power
            sleep_ms(100);
                ^ was causing issues with pico-fbw board, wasn't entirely needed anyways */
            imu_write(SYS_REGISTER, 0x00); // Use internal oscillator
            imu_write(PWR_MODE_REGISTER, PWR_MODE_NORMAL); // Use normal power mode
            sleep_ms(100);
            imu_write(AXIS_MAP_CONF_REGISTER, 0x24); // Default axis map
            imu_write(AXIS_MAP_SIGN_REGISTER, 0x00); // Default axis signs
            // If we've calibrated before, load in the axis configuration from then
            bool status;
            // if (imu_isCalibrated()) {
            if (false) { // FIXME: there are currently problems with restoring calibration data, thus data is better without it but still should be fixed
                int16_t calibrationData[11];
                for (uint i = 0; i < CONFIG_SECTOR_SIZE; i++) {
                    calibrationData[i] = (int16_t)flash_read(FLASH_SECTOR_IMU_CFG0, i);
                }
                for (uint i = 0; i < (sizeof(calibrationData) / sizeof(int16_t)) - CONFIG_SECTOR_SIZE; i++) {
                    calibrationData[i + CONFIG_SECTOR_SIZE] = (int16_t)flash_read(FLASH_SECTOR_IMU_CFG1, i);
                }
                int timeout = bno_saveCalibrationData(calibrationData, false);
                if (timeout == PICO_ERROR_GENERIC || timeout == PICO_ERROR_TIMEOUT) {
                    FBW_DEBUG_printf("[imu] ERROR: failed to save calibration data!\n");
                    return false;
                }
                bool status = bno_changeMode(MODE_NDOF); // Select NDOF mode to calibrate MAG and later obtain Euler data
                sleep_ms(800);
                unsigned char calibrationSYS, calibrationGYR, calibrationACC, calibrationMAG;
                bno_getCalibrationStatus(&calibrationSYS, &calibrationGYR, &calibrationACC, &calibrationMAG);
                while (calibrationMAG < 3) {
                    bno_getCalibrationStatus(&calibrationSYS, &calibrationGYR, &calibrationACC, &calibrationMAG);
                    FBW_DEBUG_printf("%d/3, %d/3, %d/3, %d/3\n", calibrationSYS, calibrationGYR, calibrationACC, calibrationMAG);
                    sleep_ms(1000);
                }
            } else {
                status = bno_changeMode(MODE_NDOF); // Switch to NDOF, we will calibrate shortly
            }
            // TODO: add offset calibration for BNO
            return status;
        case IMU_MODEL_MPU6050:
            imu_write(PWR_MODE_REGISTER, 0x80); // Reset power
            sleep_ms(100);
            imu_write(USER_CONTROL_REGISTER, 0b111); // Reset again
            sleep_ms(100);
            imu_write(PWR_MODE_REGISTER, 0x01); // PLL_X_GYRO is a slightly better clock source
            imu_write(INTERRUPTS_ENABLED_REGISTER, 0x00); // Disable interrupts
            imu_write(FIFO_ENABLED_REGISTER, 0x00); // Disable FIFO (using DMP FIFO)
            imu_write(ACCEL_CONFIG_REGISTER, 0x00); // Accel full scale = 2g
            imu_write(INT_PIN_CFG_REGISTER, 0x80); // Enable interrupt pin
            imu_write(PWR_MODE_REGISTER, 0x01); // Select clock source again (??)
            imu_write(SMPLRT_DIV_REGISTER, 0x04); // Sample rate = gyro rate / (1 + SMPLRT_DIV) [100Hz?]
            imu_write(CONFIG_REGISTER, 0x01); // Digital Low Pass Filter (DLPF) 188Hz
            IMU_DEBUG_printf("[imu] writing DMP to memory...\n");
            dmp_writeMemoryBlock(dmp, DMP_SIZE, 0, 0); // Write DMP to memory
            unsigned char cmd[3] = {DMP_PROG_START_ADDR, 0x04, 0x00}; // DMP program start address
            i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, cmd, 3, true, IMU_TIMEOUT_US);
            imu_write(GYRO_CONFIG_REGISTER, 0x18); // Gyro full scale = +2000 deg/s
            imu_write(USER_CONTROL_REGISTER, 0xC2); // Enable and reset DMP FIFO
            imu_write(INTERRUPTS_ENABLED_REGISTER, 0x02); // Enable RAW_DMP_INT
            // TODO: port accel and gyro calibrations from DMP lib/i2cdevlib (and use that for the return statement)

            add_repeating_timer_us(IMU_SAMPLE_RATE_US, mpu_update, NULL, &updateTimer); // Start the update timer
            return true; // FIXME: do not keep in final; return the actual status!
        default:
            return false;
    }
}

Euler imu_getRawAngles() {
    switch (imuModel) {
        case IMU_MODEL_BNO055: {
            // Get angle data from IMU
            unsigned char euler_data[6];
            int timeout0 = i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &EULER_BEGIN_REGISTER, 1, true, IMU_TIMEOUT_US);
            int timeout1 = i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, euler_data, 6, false, IMU_TIMEOUT_US);
            // Check if any I2C related errors occured, if so, set IMU as unsafe and return no data
            if (timeout0 == PICO_ERROR_GENERIC || timeout0 == PICO_ERROR_TIMEOUT || timeout1 == PICO_ERROR_GENERIC || timeout1 == PICO_ERROR_TIMEOUT) {
                setIMUSafe(false);
                return (Euler){0};
            }

            // Bit shift to combine the high byte and low byte into one signed integer
            int16_t euler_raw[3];
            for (int i = 0; i < 3; i++) {
                euler_raw[i] = (euler_data[i * 2 + 1] << 8) | euler_data[i * 2];
            }
            // Convert raw data into Euler angles
            euler.x = (float)(euler_raw[0] / 16.0);
            euler.y = (float)(euler_raw[1] / 16.0);
            euler.z = (float)(euler_raw[2] / 16.0);
            // Wrap x around if necessary (BNO handles it from 0-360 as more of a heading)
            if (euler.x > 180) {
                euler.x -= 360;
            }
            break;
        }
        case IMU_MODEL_MPU6050: {
            // FIXME: improve drift...it's really bad

            // Get current FIFO count
            fifoCount = mpu_getFIFOCount();
            if (fifoCount >= 1024) {
                // FIFO overflow, reset
                IMU_DEBUG_printf("[imu] WARNING: FIFO overflow!\n");
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
                    euler.x = (atan2(2 * q[1] * q[2] - 2 * q[0] * q[3], 2 * q[0] * q[0] + 2 * q[1] * q[1] - 1)) * (180 / M_PI);
                    euler.y = (-asin(2 * q[1] * q[3] + 2 * q[0] * q[2])) * (180 / M_PI);
                    euler.z = (atan2(2 * q[2] * q[3] - 2 * q[0] * q[1], 2 * q[0] * q[0] + 2 * q[3] * q[3] - 1)) * (180 / M_PI);
                }
            }
            break;
        }
        default: {
            setIMUSafe(false);
            return (Euler){0};
        }
    }
    return (Euler){euler.x, euler.y, euler.z};
}

Angles imu_getAngles() {
    // Get the raw Euler angles as a starting point
    Euler angles;
    switch (imuModel) {
        case IMU_MODEL_BNO055:
            angles = imu_getRawAngles(); // BNO055 is async; we must fetch the angles
            break;
        case IMU_MODEL_MPU6050:
            angles = euler; // MPU6050 is sync; we can just use the cached angles
            break;
        default:
            setIMUSafe(false);
            return (Angles){0};
    }
    
    // Map IMU axes to aircraft axes with correct directions
    float roll, pitch, yaw = -200.0f;
    switch (getCalibrationAxis(EULER_AXIS_X)) {
        case IMU_AXIS_ROLL:
            roll = shouldCompensateAxis(EULER_AXIS_X) ? -angles.x : angles.x;
            break;
        case IMU_AXIS_PITCH:
            pitch = shouldCompensateAxis(EULER_AXIS_X) ? -angles.x : angles.x;
            break;
        case IMU_AXIS_YAW:
            yaw = shouldCompensateAxis(EULER_AXIS_X) ? -angles.x : angles.x;
            break;
    }
    switch (getCalibrationAxis(EULER_AXIS_Y)) {
        case IMU_AXIS_ROLL:
            roll = shouldCompensateAxis(EULER_AXIS_Y) ? -angles.y : angles.y;
            break;
        case IMU_AXIS_PITCH:
            pitch = shouldCompensateAxis(EULER_AXIS_Y) ? -angles.y : angles.y;
            break;
        case IMU_AXIS_YAW:
            yaw = shouldCompensateAxis(EULER_AXIS_Y) ? -angles.y : angles.y;
            break;
    }
    switch (getCalibrationAxis(EULER_AXIS_Z)) {
        case IMU_AXIS_ROLL:
            roll = shouldCompensateAxis(EULER_AXIS_Z) ? -angles.z : angles.z;
            break;
        case IMU_AXIS_PITCH:
            pitch = shouldCompensateAxis(EULER_AXIS_Z) ? -angles.z : angles.z;
            break;
        case IMU_AXIS_YAW:
            yaw = shouldCompensateAxis(EULER_AXIS_Z) ? -angles.z : angles.z;
            break;
    }

    // Check for mapping errors (they really shouldn't occur and should be correctly handled before this function is even called)
    if (roll == -200.0f || pitch == -200.0f || yaw == -200.0f) {
        setIMUSafe(false);
        return (Angles){0};
    }

    return (Angles){roll, pitch, yaw};
}

bool imu_calibrate() {
    if (imuModel == IMU_MODEL_BNO055) {
        // If we are on the BNO055, first ensure we have a good calibration
        unsigned char sys, gyr, acc, mag;
        bno_getCalibrationStatus(&sys, &gyr, &acc, &mag);
        FBW_DEBUG_printf("[imu] BNO055 calibration status: SYS: %d/3, GYR: %d/3, ACC: %d/3, MAG: %d/3\n", sys, gyr, acc, mag);
        if (sys < 3) {
            FBW_DEBUG_printf("[imu] sensor calibration must be performed!\n");
            error_throw(ERROR_IMU, ERROR_LEVEL_STATUS, 500, 250, true, "");
            while (sys < 3 || acc < 1) {
                bno_getCalibrationStatus(&sys, &gyr, &acc, &mag);
                FBW_DEBUG_printf("%d/3, %d/3, %d/3, %d/3\n", sys, gyr, acc, mag);
                sleep_ms(2000);
            }
            FBW_DEBUG_printf("[imu] sensor calibration complete! saving calibration...\n");
            bno_changeMode(MODE_CONFIG); // Must be in CONFIG mode to read calibration data
            int16_t data[11];
            int timeout = bno_getCalibrationData(data);
            if (timeout != PICO_ERROR_GENERIC && timeout != PICO_ERROR_TIMEOUT) {
                // Convert to floats so we can store in flash
                float imu0[CONFIG_SECTOR_SIZE] = {FLAG_IMU};
                for (uint i = 0; i < CONFIG_SECTOR_SIZE; i++) {
                    imu0[i] = (float)data[i];
                }
                float imu1[CONFIG_SECTOR_SIZE];
                for (uint i = 0; i < (sizeof(data) / sizeof(int16_t)) - CONFIG_SECTOR_SIZE; i++) {
                    imu1[i] = (float)data[i + CONFIG_SECTOR_SIZE];
                }
                flash_write(FLASH_SECTOR_IMU_CFG0, imu0);
                flash_write(FLASH_SECTOR_IMU_CFG1, imu1);
            } else {
                FBW_DEBUG_printf("[imu] failed to read BNO055 calibration data!\n");
                return false;
            }
            // Write data back as a test
            int16_t test[11];
            for (uint i = 0; i < CONFIG_SECTOR_SIZE; i++) {
                test[i] = (int16_t)flash_read(FLASH_SECTOR_IMU_CFG0, i);
            }
            for (uint i = 0; i < (sizeof(test) / sizeof(int16_t)) - CONFIG_SECTOR_SIZE; i++) {
                test[i + CONFIG_SECTOR_SIZE] = (int16_t)flash_read(FLASH_SECTOR_IMU_CFG1, i);
            }
            timeout = bno_saveCalibrationData(test, true);
            if (timeout != PICO_ERROR_GENERIC && timeout != PICO_ERROR_TIMEOUT) {
                FBW_DEBUG_printf("[imu] BNO055 calibration data saved!\n");
                error_clear(ERROR_IMU, false);
                bno_changeMode(MODE_NDOF); // Change back to NDOF mode for next step
            } else {
                FBW_DEBUG_printf("[imu] failed to test save BNO055 calibration data!\n");
                return false;
            }
        }
    }
    return true; // TODO remove, just for testing for now
    FBW_DEBUG_printf("[imu] starting imu angle mapping calibration\n");
    error_throw(ERROR_IMU, ERROR_LEVEL_STATUS, 500, 100, true, ""); // Blink for calibration status
    float calibration_data[CONFIG_SECTOR_SIZE] = {FLAG_IMU, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; // {flag, x, y, z, x_dir, y_dir, z_dir}
    // Complete all axes' calibration
    IMUCalibrationState state = CALIBRATION_STATE_ROLL;
    while (state != CALIBRATION_STATE_COMPLETE) {
        EulerAxis movedAxis = EULER_AXIS_NONE;
        float movedAngle;
        // Loop until the user has moved the IMU to the next axis
        while (movedAxis == EULER_AXIS_NONE) {
            Euler angles = imu_getRawAngles();
            if (fabsf(angles.x) > 20) {
                movedAxis = EULER_AXIS_X;
                movedAngle = angles.x;
            } else if (fabsf(angles.y) > 20) {
                movedAxis = EULER_AXIS_Y;
                movedAngle = angles.y;
            } else if (fabsf(angles.z) > 20) {
                movedAxis = EULER_AXIS_Z;
                movedAngle = angles.z;
            }
        }

        // Write correct calibration data based on what we see
        switch (state) {
            case CALIBRATION_STATE_ROLL:
                calibration_data[(uint)movedAxis] = IMU_AXIS_ROLL;
                IMU_DEBUG_printf("[imu] roll axis is axis %d\n", movedAxis);
                if (movedAngle < 0) {
                    // This indicates a left roll (we are expecting a right), so compensate
                    calibration_data[(uint)movedAxis + 3] = 1.0f;
                }
                break;
            case CALIBRATION_STATE_PITCH:
                calibration_data[(uint)movedAxis] = IMU_AXIS_PITCH;
                IMU_DEBUG_printf("[imu] pitch axis is axis %d\n", movedAxis);
                if (movedAngle < 0) {
                    // Indicates a pitch down, compensate
                    calibration_data[(uint)movedAxis + 3] = 1.0f;
                }
                break;
            case CALIBRATION_STATE_YAW:
                calibration_data[(uint)movedAxis] = IMU_AXIS_YAW;
                IMU_DEBUG_printf("[imu] yaw axis is axis %d\n", movedAxis);
                if (movedAngle > 0) {
                    // Indicates a right yaw, compensate
                    calibration_data[(uint)movedAxis + 3] = 1.0f;
                }
                break;
        }

        // 3 blinks to signify an axis has been recognized
        error_throw(ERROR_IMU, ERROR_LEVEL_STATUS, 250, 100, true, "");
        absolute_time_t resume = make_timeout_time_ms(750);
        while (!time_reached(resume)) {
            // We need to ensure the angles are always being fetched as the MPU isn't async, so we can't simply sleep
            imu_getRawAngles();
        }
        error_clear(ERROR_IMU, false);
        // Wait for user to re-center, and then advance to the next axis
        resume = make_timeout_time_ms(2000);
        while (!time_reached(resume)) {
            imu_getRawAngles();
        }
        error_throw(ERROR_IMU, ERROR_LEVEL_STATUS, 500, 100, true, "");
        state++;
    }
    error_clear(ERROR_IMU, false);

    // Check data before writing to flash
    if (calibration_data[1] != calibration_data[2] && calibration_data[1] != calibration_data[3] && calibration_data[2] != calibration_data[1] && calibration_data[2] != calibration_data[3] && calibration_data[3] != calibration_data[1] && calibration_data[3] != calibration_data[2]) {
        flash_write(FLASH_SECTOR_IMU_MAP, calibration_data);
        FBW_DEBUG_printf("[imu] imu angle mapping calibration complete\n");
        return true;
    } else {
        FBW_DEBUG_printf("[imu] one or more axes were the same, calibration failed!\n");
        return false;
    }
}

bool imu_isCalibrated() {
    // Read the flags as well as ensure values make sense
    if (flash_read(FLASH_SECTOR_IMU_MAP, 0) == FLAG_IMU && flash_read(FLASH_SECTOR_IMU_CFG0, 0 == FLAG_IMU)) {
        float data[6];
        for (uint8_t i = 1; i <= 6; i++) {
            data[i - 1] = flash_read(FLASH_SECTOR_IMU_MAP, i);
            if (!isfinite(data[i - 1])) {
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}
