/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdio.h>
// TODO include stdint and pico/types if it ends up being needed
#include "../sys/config.h"

#include "hardware/i2c.h"

#include "imu.h"

#include "baro.h"

/* --- All baros --- */

// These are all populated when baro_init() is called
static BaroModel baroModel;
static unsigned char CHIP_REGISTER, ID_REGISTER, CHIP_ID;
static bool baroUsable = false;

/* --- DPS310 --- */



/* --- All baros --- */

int baro_init() {
    baroModel = config.sensors.baroModel;
    if (platform_is_fbw()) {
        baroModel = BARO_MODEL_DPS310;
    }
    if (config.debug.debug_fbw) printf("[baro] searching for ");
    switch (baroModel) {
        case BARO_MODEL_DPS310:
            CHIP_REGISTER = DPS_CHIP_REGISTER;
            ID_REGISTER = DPS_ID_REGISTER;
            CHIP_ID = DPS_CHIP_ID;
            if (config.debug.debug_fbw) printf("DPS310\n");
            break;
    }
    if (config.debug.debug_imu) printf("[baro] checking ID (writing 0x%02X [ID_REGISTER] to 0x%02X [CHIP_REGISTER]) with timeout of %dus...\n", ID_REGISTER, CHIP_REGISTER, BARO_TIMEOUT_US);
    int result = i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &ID_REGISTER, 1, true, BARO_TIMEOUT_US);
    if (result == 1) { // Bytes written should be 1
        unsigned char id;
        result = i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, &id, 1, false, BARO_TIMEOUT_US);
        if (result == 1) {
            if (config.debug.debug_imu) printf("[baro] ID: 0x%02X\n", id);
            if (id == CHIP_ID) {
                if (config.debug.debug_imu) printf("[baro] ID ok\n");
                result = 0;
                baroUsable = true;
            } else {
                if (config.debug.debug_imu) printf("[baro] ERROR: ID does not match expected value\n");
                result = 1;
            }
        } else {
            if (config.debug.debug_fbw) printf("[baro] ERROR: unable to read ID\n");
        }
    } else {
        if (config.debug.debug_fbw) printf("[baro] ERROR: address not acknowledged (no/wrong device present?)\n");
    }
    return result;
}

// TODO: funcs for getting alt (and whatever else may be needed to do that...calibration?)
// also once baro is done, replace gps.alt with baro.alt when applicable

bool canUseBaro() { return baroUsable; }
