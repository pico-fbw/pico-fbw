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
*/
static inline int imu_write(unsigned char addr, unsigned char val) {
    unsigned char c[2] = {addr, val};
    return i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, c, 2, true, IMU_TIMEOUT_US);
}

static inline IMUAxis getCalibrationAxis(EulerAxis axis) { return (IMUAxis)(flash_read(FLASH_SECTOR_IMU, (uint)axis)); }
static inline bool shouldCompensateAxis(EulerAxis axis) { return (bool)(flash_read(FLASH_SECTOR_IMU, (uint)(axis + 3))); }

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
    i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &OPR_MODE_REGISTER, 1, true, IMU_TIMEOUT_US);
    i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, &currentMode, 1, false, IMU_TIMEOUT_US);
    if (currentMode == mode) {
        return true;
    } else {
        IMU_DEBUG_printf("[imu] failed to change mode, mode is still 0x%02X, supposed to be 0x%02X\n", currentMode, mode);
        return false;
    }
}

static uint16_t fifoCount;
static uint8_t fifoBuf[DMP_PACKET_SIZE];

struct repeating_timer updateTimer;

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
            // Check the calibration status
            unsigned char calibrationStatus;
            i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &CALIBRATION_REGISTER, 1, true, IMU_TIMEOUT_US);
            i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, &calibrationStatus, 1, false, IMU_TIMEOUT_US);
            uint8_t calibrationSYS = (calibrationStatus >> 6) & 0x03;
            if (calibrationSYS < 3) {
                uint8_t calibrationGYR = (calibrationStatus >> 4) & 0x03;
                uint8_t calibrationACC = (calibrationStatus >> 2) & 0x03;
                uint8_t calibrationMAG = calibrationStatus & 0x03;
                error_throw(ERROR_IMU, ERROR_LEVEL_WARN, 1000, 0, false, "IMU calibration is not optimal.");
                FBW_DEBUG_printf("[imu] calibration status: SYS: %d, GYR: %d, ACC: %d, MAG: %d\n", calibrationSYS, calibrationGYR, calibrationACC, calibrationMAG);
            }
            return bno_changeMode(MODE_NDOF); // Select NDOF mode to obtain Euler data
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
            uint8_t cmd[3] = {DMP_PROG_START_ADDR, 0x04, 0x00}; // DMP program start address
            i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, cmd, 3, true, IMU_TIMEOUT_US);
            imu_write(GYRO_CONFIG_REGISTER, 0x18); // Gyro full scale = +2000 deg/s
            imu_write(USER_CONTROL_REGISTER, 0xC2); // Enable and reset DMP FIFO
            imu_write(INTERRUPTS_ENABLED_REGISTER, 0x02); // Enable RAW_DMP_INT
            // TODO: port accel and gyro calibrations from DMP lib/i2cdevlib (and use that for the return statement)

            add_repeating_timer_us(IMU_SAMPLE_RATE_US, mpu_update, NULL, &updateTimer); // Start the update timer
            return true; // do not keep in final; return the actual status!
        default:
            return false;
    }
}

Euler imu_getRawAngles() {
    switch (imuModel) {
        case IMU_MODEL_BNO055: {
            // Get angle data from IMU
            uint8_t euler_data[6];
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

inertialAngles imu_getAngles() {
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
            return (inertialAngles){0};
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
        return (inertialAngles){0};
    }

    return (inertialAngles){roll, pitch, yaw};
}

bool imu_calibrate() {
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
        flash_write(FLASH_SECTOR_IMU, calibration_data);
        FBW_DEBUG_printf("[imu] imu angle mapping calibration complete\n");
        return true;
    } else {
        FBW_DEBUG_printf("[imu] one or more axes were the same, calibration failed!\n");
        return false;
    }
}

bool imu_isCalibrated() {
    // Read the flag as well as ensure values make sense
    if (flash_read(FLASH_SECTOR_IMU, 0) == FLAG_IMU) {
        float data[6];
        for (uint8_t i = 1; i <= 6; i++) {
            data[i - 1] = flash_read(FLASH_SECTOR_IMU, i);
            if (!isfinite(data[i - 1])) {
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}
