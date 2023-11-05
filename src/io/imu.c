/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "pico/types.h"

#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "flash.h"
#include "platform.h"

#include "../modes/modes.h"

#include "../sys/log.h"

#include "imu.h"

// TODO: can I possibly move AHRS implementation over to the Pico?
// I tried this before and it didn't work but maybe try again once other things are implemented
// have some filtering logic for a 9-axis sensor and offload to second core, as an idea

typedef enum IMUAxis {
    IMU_AXIS_ROLL,
    IMU_AXIS_PITCH,
    IMU_AXIS_YAW,
    IMU_AXIS_NONE
} IMUAxis;

typedef enum EulerAxis {
    EULER_AXIS_X,
    EULER_AXIS_Y,
    EULER_AXIS_Z,
    EULER_AXIS_NONE
} EulerAxis;

/* --- All IMUs --- */

// These are all populated when imu_init() is called
static IMUModel imuModel = IMU_MODEL_NONE;
static uint CHIP_FREQ_KHZ;
static unsigned char CHIP_REGISTER, ID_REGISTER, CHIP_ID;

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

// FIXME: these are ovverriden for now as I expect them to be gone shortly after the 15 millionth imu rewrite
// static inline IMUAxis getCalibrationAxis(EulerAxis axis) { return (IMUAxis)(flash_readFloat(FLOAT_SECTOR_IMU_MAP, (uint)axis)); }
// static inline bool shouldCompensateAxis(EulerAxis axis) { return (bool)(flash_readFloat(FLOAT_SECTOR_IMU_MAP, (uint)(axis + 3))); }
static inline IMUAxis getCalibrationAxis(EulerAxis axis) { return IMU_AXIS_NONE; }
static inline bool shouldCompensateAxis(EulerAxis axis) { return false; }

/* --- BNO055 --- */

/**
 * Changes the working mode of the BNO055.
 * @param mode The code of the mode to change into (for example, 0x0C for NDOF).
 * @return true if success, false if failure.
*/
static bool bno_changeMode(unsigned char mode) {
    if (print.imu) printf("[imu] changing to mode 0x%02X\n", mode);
    imu_write(OPR_MODE_REGISTER, mode);
    sleep_ms(100);
    // Check to ensure mode has changed properly by reading it back
    unsigned char currentMode;
    i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &OPR_MODE_REGISTER, sizeof(OPR_MODE_REGISTER), true, IMU_TIMEOUT_US);
    i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, &currentMode, sizeof(currentMode), false, IMU_TIMEOUT_US);
    if (currentMode == mode) {
        return true;
    } else {
        if (print.imu) printf("[imu] failed to change mode, mode is still 0x%02X, supposed to be 0x%02X\n", currentMode, mode);
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
 * @param data Pointer to an array (at least 20 elements) to store the calibration data.
 * @return Number of bytes read (should be 40), or PICO_ERROR_GENERIC if address not acknowledged, no device present.
 * PICO_ERROR_TIMEOUT if a timeout occured.
 * @note You must be in CONFIG mode to read this data.
*/
static int bno_getCalibrationData(int16_t *data) {
    unsigned char buf[40];
    int timeout = i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &SIC_MATRIX_0_LSB_REGISTER, sizeof(SIC_MATRIX_0_LSB_REGISTER), true, IMU_TIMEOUT_US);
    if (timeout != sizeof(SIC_MATRIX_0_LSB_REGISTER)) return timeout;
    timeout = i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, buf, sizeof(buf), false, IMU_TIMEOUT_US);
    for (uint i = 0; i < (sizeof(buf) / 2); i++) {
        data[i] = (buf[i * 2 + 1] << 8) | buf[i * 2];
        if (print.imu) printf("[BNO055] read 0x%04X from LSB 0x%02X, MSB 0x%02X\n", data[i], buf[i * 2 + 1], buf[i * 2]);
    }
    return timeout;
}

/**
 * Saves sensor calibration data to the BNO055.
 * @param data Pointer to an array (at least 11 elements) holding the calibration data to store.
 * @param mag Whether to save the magnetometer calibration data or not.
 * @return Number of bytes written (23 for mag and 14 for no mag), or PICO_ERROR_GENERIC if address not acknowledged, no device present.
 * PICO_ERROR_TIMEOUT if a timeout occured.
 * @note You must be in CONFIG mode to write this data.
*/
static int bno_saveCalibrationData(int16_t *data, bool mag) {
    unsigned char buf[41];
    buf[0] = SIC_MATRIX_0_LSB_REGISTER;
    for (uint i = 1; i < 20; i++) {
        buf[i * 2] = data[i - 1] & 0xFF; // Store LSB
        buf[i * 2 + 1] = (data[i - 1] >> 8) & 0xFF; // Store MSB
        if (print.imu) printf("[BNO055] write 0x%04X to LSB 0x%02X, MSB 0x%02X\n", data[i - 1], buf[i * 2 + 1], buf[i * 2]);
    }
    return i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, buf, sizeof(buf), true, IMU_TIMEOUT_US);
}

/* --- All IMUs --- */

int imu_init() {
    // Initialization steps are the same for all IMUs, only some addresses change, so populate those now based on whatever is selected in the config
    imuModel = IMU_MODEL_BNO055;
    if (platform_is_fbw()) {
        imuModel = IMU_MODEL_BNO055;
    }
    if (print.fbw) printf("[imu] initializing ");
    if (IMU_I2C == i2c0) {
        if (print.fbw) printf("i2c0\n");
    } else if (IMU_I2C == i2c1) {
        if (print.fbw) printf("i2c1\n");
    } else {
        if (print.fbw) printf("\n");
    }
    i2c_init(IMU_I2C, CHIP_FREQ_KHZ * 1000);
    gpio_set_function((uint)flash.pins[PINS_AAHRS_SDA], GPIO_FUNC_I2C);
    gpio_set_function((uint)flash.pins[PINS_AAHRS_SCL], GPIO_FUNC_I2C);
    gpio_pull_up((uint)flash.pins[PINS_AAHRS_SDA]);
    gpio_pull_up((uint)flash.pins[PINS_AAHRS_SCL]);
    // Query the ID register for expected values to confirm identity & check comms
    if (print.fbw) printf("[imu] searching for ");
    switch (imuModel) {
        case IMU_MODEL_BNO055:
            CHIP_FREQ_KHZ = BNO_CHIP_FREQ_KHZ;
            CHIP_REGISTER = BNO_CHIP_REGISTER;
            ID_REGISTER = BNO_ID_REGISTER;
            CHIP_ID = BNO_CHIP_ID;
            if (print.fbw) printf("BNO055\n");
            break;
    }
    if (print.imu) printf("[imu] checking ID (writing 0x%02X [ID_REGISTER] to 0x%02X [CHIP_REGISTER]) with timeout of %dus...\n", ID_REGISTER, CHIP_REGISTER, IMU_TIMEOUT_US);
    int result = i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &ID_REGISTER, 1, true, IMU_TIMEOUT_US);
    if (result == 1) { // 1 byte should be written
        unsigned char id;
        result = i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, &id, 1, false, IMU_TIMEOUT_US);
        if (result == 1) {
            if (print.imu) printf("[imu] ID: 0x%02X\n", id);
            if (id == CHIP_ID) {
                if (print.imu) printf("[imu] ID ok\n");
                result = 0;
            } else {
                if (print.imu) printf("[imu] ERROR: ID does not match expected value\n");
                result = 1;
            }
        } else {
            if (print.fbw) printf("[imu] ERROR: unable to read ID\n");
        }
    } else {
        if (print.fbw) printf("[imu] ERROR: address not acknowledged (no/wrong device present?)\n");
    }
    return result;
}

void imu_deinit() { i2c_deinit(IMU_I2C); }

bool imu_configure() {
    if (print.fbw) printf("[imu] configuring sensor\n");
    switch (imuModel) {
        case IMU_MODEL_BNO055:
            imu_write(SYS_REGISTER, 0x00); // Use internal oscillator
            imu_write(PWR_MODE_REGISTER, PWR_MODE_NORMAL); // Use normal power mode
            sleep_ms(100);
            imu_write(UNIT_SEL_REGISTER, 0x01); // Select units (Windows orientation, °C, °, °/s, mg)
            bool status;
            /* FIXME: there are currently problems with restoring calibration data, thus data is better without it but still should be fixed
            // If we've calibrated before, load in the axis configuration from then
            if (imu_isCalibrated()) {
                if ((IMUModel)flash_readFloat(FLOAT_SECTOR_IMU_MAP, 1) != IMU_MODEL_BNO055) {
                    if (print.fbw) printf("[imu] WARNING: calibration was completed for a different IMU! rebooting to re-calibrate now...\n");
                    // Erase IMU calibration and then reboot
                    float erase[FLOAT_SECTOR_SIZE] = { [0 ... FLOAT_SECTOR_SIZE - 1] = INFINITY };
                    flash_writeFloat(FLOAT_SECTOR_IMU_MAP, erase, true);
                    platform_reboot(REBOOT_FAST);
                }
                imu_write(AXIS_MAP_CONF_REGISTER, (unsigned char)flash_readFloat(FLOAT_SECTOR_IMU_MAP, 2)); // Axis map
                imu_write(AXIS_MAP_SIGN_REGISTER, (unsigned char)flash_readFloat(FLOAT_SECTOR_IMU_MAP, 3)); // Axis signs
                int16_t calibrationData[11];
                for (uint i = 0; i < FLOAT_SECTOR_SIZE; i++) {
                    calibrationData[i] = (int16_t)flash_readFloat(FLOAT_SECTOR_IMU_CFG0, i);
                }
                for (uint i = 0; i < (sizeof(calibrationData) / sizeof(int16_t)) - FLOAT_SECTOR_SIZE; i++) {
                    calibrationData[i + FLOAT_SECTOR_SIZE] = (int16_t)flash_readFloat(FLOAT_SECTOR_IMU_CFG1, i);
                }
                int timeout = bno_saveCalibrationData(calibrationData, false);
                if (timeout != 14) {
                    if (print.fbw) printf("[imu] ERROR: failed to save calibration data!\n");
                    return false;
                }
                bool status = bno_changeMode(MODE_NDOF); // Select NDOF mode to calibrate MAG and later obtain Euler data
                sleep_ms(800);
                unsigned char calibrationSYS, calibrationGYR, calibrationACC, calibrationMAG;
                bno_getCalibrationStatus(&calibrationSYS, &calibrationGYR, &calibrationACC, &calibrationMAG);
                while (calibrationSYS < 3) {
                    bno_getCalibrationStatus(&calibrationSYS, &calibrationGYR, &calibrationACC, &calibrationMAG);
                    if (print.fbw) printf("All: %d/3, Gyr: %d/3, Acc: %d/3, Mag: %d/3\n", calibrationSYS, calibrationGYR, calibrationACC, calibrationMAG);
                    sleep_ms(1000);
                }
            } else {
                // Set default axis mapping and signs as we have not calibrated those yet
                imu_write(AXIS_MAP_CONF_REGISTER, 0x24);
                imu_write(AXIS_MAP_SIGN_REGISTER, 0x00);
                status = bno_changeMode(MODE_NDOF); // Switch to NDOF, we will calibrate shortly (called in main)
            }
            return status;
            */
            return bno_changeMode(MODE_NDOF);
        default:
            return false;
    }
}

Euler imu_getRawAngles() {
    static Euler euler;
    switch (imuModel) {
        case IMU_MODEL_BNO055: {
            unsigned char euler_data[6];
            int timeout = i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &EULER_BEGIN_REGISTER, sizeof(EULER_BEGIN_REGISTER), true, IMU_TIMEOUT_US);
            if (timeout != sizeof(EULER_BEGIN_REGISTER)) goto i2cErr;
            timeout = i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, euler_data, count_of(euler_data), false, IMU_TIMEOUT_US);
            if (timeout != count_of(euler_data)) goto i2cErr;

            // Bit shift to combine the high byte and low byte into one signed integer
            int16_t euler_raw[3];
            for (int i = 0; i < count_of(euler_raw); i++) {
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
        default: {
            if (print.fbw) printf("[imu] ERROR: unknown IMU model!\n");
            setIMUSafe(false);
            return (Euler){0};
        }
    }
    return (Euler){euler.x, euler.y, euler.z};
    i2cErr:
        if (print.imu) printf("[imu] ERROR: i2c read error occured!\n");
        setIMUSafe(false);
        return (Euler){0};
}

Angles imu_getAngles() {
    float roll, pitch, yaw = INFINITY;
    // Get the raw Euler angles as a starting point, then map based on IMU
    Euler angles;
    switch (imuModel) {
        case IMU_MODEL_BNO055:
            angles = imu_getRawAngles();
            roll = angles.z;
            pitch = angles.y;
            yaw = angles.x;
            break;
        default:
            if (print.fbw) printf("[imu] ERROR: unknown IMU model!\n");
            setIMUSafe(false);
            return (Angles){INFINITY};
    }
    return (Angles){roll, pitch, yaw};
}

bool imu_calibrate() {
    switch (imuModel) {
        case IMU_MODEL_BNO055: {
            // First, ensure we have a good calibration
            unsigned char sys, gyr, acc, mag;
            do {
                log_message(INFO, "Sensor calibration must be performed!", 500, 250, true);
                bno_getCalibrationStatus(&sys, &gyr, &acc, &mag);
                if (print.fbw) printf("All: %d/3, Gyr: %d/3, Acc: %d/3, Mag: %d/3\n", sys, gyr, acc, mag);
                sleep_ms(2000);
            } while (sys < 3);
            log_clear(INFO);
            if (print.fbw) printf("[imu] sensor calibration complete! saving calibration...\n");
            bno_changeMode(MODE_CONFIG); // Must be in CONFIG mode to read calibration data
            sleep_ms(200);
            int16_t data[11];
            int timeout = bno_getCalibrationData(data);
            // FIXME: all flash code commented out here, for the same reason as before--will probably be replaced, but don't delete
            // this comment until we are certain about it!
            if (timeout != 22) {
                // Convert to floats so we can store in flash
                float imu0[FLOAT_SECTOR_SIZE] = {FLAG_IMU};
                for (uint i = 0; i < FLOAT_SECTOR_SIZE; i++) {
                    imu0[i] = (float)data[i];
                }
                float imu1[FLOAT_SECTOR_SIZE];
                for (uint i = 0; i < (sizeof(data) / sizeof(int16_t)) - FLOAT_SECTOR_SIZE; i++) {
                    imu1[i] = (float)data[i + FLOAT_SECTOR_SIZE];
                }
                // flash_writeFloat(FLOAT_SECTOR_IMU_CFG0, imu0, false);
                // flash_writeFloat(FLOAT_SECTOR_IMU_CFG1, imu1, false);
                // flash_flushCache();
            } else {
                if (print.fbw) printf("[imu] failed to read BNO055 calibration data!\n");
                return false;
            }
            // Write data back as a test
            int16_t test[11];
            for (uint i = 0; i < FLOAT_SECTOR_SIZE; i++) {
                // test[i] = (int16_t)flash_readFloat(FLOAT_SECTOR_IMU_CFG0, i);
            }
            for (uint i = 0; i < (sizeof(test) / sizeof(int16_t)) - FLOAT_SECTOR_SIZE; i++) {
                // test[i + FLOAT_SECTOR_SIZE] = (int16_t)flash_readFloat(FLOAT_SECTOR_IMU_CFG1, i);
            }
            timeout = bno_saveCalibrationData(test, true);
            if (timeout == 23) {
                if (print.fbw) printf("[imu] BNO055 calibration data saved!\n");
                bno_changeMode(MODE_NDOF); // Change back to NDOF mode for next step
            } else {
                if (print.fbw) printf("[imu] failed to test save BNO055 calibration data!\n");
                return false;
            }
            // Now for the mapping stage of calibration
            if (print.fbw) printf("[imu] starting imu axis mapping calibration\n");
            sleep_ms(3000); // Give the user a bit of a breather
            uint calibration[6] = { [0 ... 5] = 0 }; // {x, y, z, x_dir, y_dir, z_dir}
            EulerAxis currentAxis = EULER_AXIS_X;
            while (currentAxis < EULER_AXIS_NONE) {
                /* So, it took me about a millenia to figure this out, and I'll explain it to the best of my ability--
                the BNO055 refers to its axes differently in fusion mode than in other modes (like the mapping here).
                For example, Z is heading in the mapping config but it's X when getting Euler data...very odd.
                For this, take a look at page 26 of the BNO055 datasheet for a visual reference...thanks for the confusion Bosch ._. */
                switch (currentAxis) {
                    case EULER_AXIS_X: // X = roll
                        log_message(INFO, "Roll right", 500, 100, true);
                        break;
                    case EULER_AXIS_Y: // Y = pitch
                        log_message(INFO, "Pitch up", 500, 100, true);
                        break;
                    case EULER_AXIS_Z: // Z = heading
                        log_message(INFO, "Yaw left", 500, 100, true);
                        break;
                }
                // TODO: this calibration can still be a bit iffy in some circumstances (although works in most), fix when able?
                // Obtain a Euler of the current positional data to compare to (so we're working with relative and not absolute data)
                Euler original = imu_getRawAngles();
                // Loop until the user has moved the IMU to the next axis
                EulerAxis movedAxis = EULER_AXIS_NONE;
                int movedAngle;
                while (movedAxis == EULER_AXIS_NONE) {
                    Euler angles = imu_getRawAngles();
                    if (abs(ANGLE_DIFFERENCE((int)original.x, (int)angles.x)) > 20) {
                        movedAxis = EULER_AXIS_Z;
                        movedAngle = ANGLE_DIFFERENCE((int)original.x, (int)angles.x);
                    } else if (abs(ANGLE_DIFFERENCE((int)original.y, (int)angles.y)) > 20) {
                        movedAxis = EULER_AXIS_X;
                        movedAngle = ANGLE_DIFFERENCE((int)original.y, (int)angles.y);
                    } else if (abs(ANGLE_DIFFERENCE((int)original.z, (int)angles.z)) > 20) {
                        movedAxis = EULER_AXIS_Y;
                        movedAngle = ANGLE_DIFFERENCE((int)original.z, (int)angles.z);
                    }
                }

                // Write correct calibration data based on what we observed
                calibration[currentAxis] = movedAxis;
                // All of the movements the user was instructed to do should result in positive output, so we should reverse the axis if it didn't
                calibration[currentAxis + 3] = (movedAngle < 0); 
                if (print.imu) printf("[imu] moved axis was %d @ %ddeg (reverse: %d)\n", movedAxis, movedAngle, calibration[currentAxis + 3]);

                // Signify an axis has been recognized
                log_clear(INFO);
                log_message(INFO, "Return to center", 250, 100, true);
                sleep_ms(750);
                log_clear(INFO);
                sleep_ms(3000);
                currentAxis++;
            }

            // Validate
            for (uint i = 0; i < 2; i++) {
                for (uint j = i + 1; j < 3; j++) {
                    if (calibration[i] == calibration[j]) {
                        if (print.fbw) printf("[imu] one or more axes were the same, calibration failed!\n");
                        return false;
                    }
                }
            }
            // Convert to BNO055 axis mapping
            unsigned char map = 0;
            map |= (calibration[2] & 3) << 4;
            map |= (calibration[1] & 3) << 2;
            map |= (calibration[0] & 3);
            unsigned char sign = 0;
            sign |= (calibration[3] & 1) << 2;
            sign |= (calibration[4] & 1) << 1;
            sign |= (calibration[5] & 1);
            if (print.imu) printf("[imu] final map is 0x%02X, final sign is 0x%02X\n", map, sign);
            // Write to flash
            float imu[FLOAT_SECTOR_SIZE] = {FLAG_IMU, imuModel, map, sign};
            // flash_writeFloat(FLOAT_SECTOR_IMU_MAP, imu, true);
            break;
        }
        default: {
            if (print.fbw) printf("[imu] ERROR: unable to calibrate this model!\n");
            return false;
        }
    }
    return true;
}

bool imu_isCalibrated() {
    // FIXME: yeah same thing here
    // Read the flag as well as ensure values make sense
    // if (flash_readFloat(FLOAT_SECTOR_IMU_MAP, 0) == FLAG_IMU) {
    //     for (uint8_t i = 0; i <= 3; i++) {
    //         if (!isfinite(flash_readFloat(FLOAT_SECTOR_IMU_MAP, i))) {
    //             return false;
    //         }
    //     }
        return true;
    // } else {
    //     return false;
    // }
}
