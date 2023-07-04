/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdio.h>

#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "../modes/modes.h"
#include "../config.h"

#include "imu.h"

/**
 * A low(er)-level method that writes a value directly to the IMU over I2C.
 * @param address The address to write to.
 * @param value The value to write.
 * @return Number of bytes written, or PICO_ERROR_GENERIC if address not acknowledged, no device present.
*/
static int imu_write(uint8_t address, uint8_t value) {
    uint8_t cmd[2] = {address, value};
    return i2c_write_blocking(IMU_I2C, CHIP_REGISTER, cmd, 2, true);
}

int imu_init() {
    FBW_DEBUG_printf("[i2c] initializing i2c subsystem\n");
    #ifdef IMU_BNO055
		i2c_init(IMU_I2C, CHIP_FREQ_KHZ * 1000);
		gpio_set_function(IMU_SDA_PIN, GPIO_FUNC_I2C);
		gpio_set_function(IMU_SCL_PIN, GPIO_FUNC_I2C);
		gpio_pull_up(IMU_SDA_PIN);
		gpio_pull_up(IMU_SCL_PIN);
		bi_decl(bi_2pins_with_func(IMU_SDA_PIN, IMU_SCL_PIN, GPIO_FUNC_I2C));
		// Query the ID register for expected values to confirm identity/check comms
        FBW_DEBUG_printf("[imu] searching for BNO055\n");
		uint8_t id;
		int result = i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &ID_REGISTER, 1, true, 1000000);
		if (result == PICO_ERROR_GENERIC) {
			return PICO_ERROR_GENERIC;
		} else if (result == PICO_ERROR_TIMEOUT) {
			return PICO_ERROR_TIMEOUT;
		} else {
            result = i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, &id, 1, false, 1000000);
            if (result == PICO_ERROR_GENERIC) {
                return PICO_ERROR_GENERIC;
            } else if (result== PICO_ERROR_TIMEOUT) {
                return PICO_ERROR_TIMEOUT;
            } else {
                if (id == CHIP_ID) {
                    return 0;
                } else {
                    return 1;
                }
            }
		}
    #else
		#error No IMU module was defined.
    #endif
}

void imu_deinit() {
    i2c_deinit(IMU_I2C);
}

bool imu_configure() {
    FBW_DEBUG_printf("[imu] configuring IMU, watch for config data dump\n");
    #ifdef IMU_BNO055
        FBW_DEBUG_printf("[imu] config: using IMU internal oscillator\n");
        imu_write(SYS_REGISTER, 0x40);
        FBW_DEBUG_printf("[imu] config: resetting interrupts\n");
        imu_write(SYS_REGISTER, 0x01);
        FBW_DEBUG_printf("[imu] config: setting normal power mode\n");
        imu_write(PWR_MODE_REGISTER, PWR_MODE_NORMAL);
        sleep_ms(50);
        FBW_DEBUG_printf("[imu] config: settinf default axis configuration\n");
        imu_write(AXIS_MAP_CONF_REGISTER, 0x24);
        FBW_DEBUG_printf("[imu] config: setting default axis signs\n");
        imu_write(AXIS_MAP_SIGN_REGISTER, 0x00);
        FBW_DEBUG_printf("[imu] config: setting acceleration units to milli-Gs\n");
        imu_write(0x3B, 0b00000001);
        sleep_ms(30);
        FBW_DEBUG_printf("[imu] config: attempting to set NDOF mode\n");
        return imu_changeMode(MODE_NDOF);
    #else
        #warning No sutable IMU could be found for configuration, this may cause problems later.
    #endif
}

inertialAngles imu_getAngles() {
    // Get angle data from IMU
    uint8_t euler_data[6];
    int timeout0 = i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &EULER_BEGIN_REGISTER, 1, true, 10000);
    int timeout1 = i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, euler_data, 6, false, 10000);
    // Check if any I2C related errors occured, if so, set IMU as unsafe and return no data
    if (timeout0 == PICO_ERROR_GENERIC || timeout0 == PICO_ERROR_TIMEOUT || timeout1 == PICO_ERROR_GENERIC || timeout1 == PICO_ERROR_TIMEOUT) {
        setIMUSafe(false);
        return (inertialAngles){0};
    }
    // Perform the necessary bit shifts to combine the high byte and low byte into one signed integer
    int16_t heading = (euler_data[1] << 8) | euler_data[0];
    int16_t roll = (euler_data[3] << 8) | euler_data[2];
    int16_t pitch = (euler_data[5] << 8) | euler_data[4];

    // Convert raw data into angles
    float heading_degrees = (float)heading / 16.0;
    float roll_degrees = (float)roll / 16.0;
    float pitch_degrees = (float)pitch / 16.0;
    float yaw_degrees = heading_degrees;
    if (yaw_degrees >= 180) {
        yaw_degrees -= 360;
    }
    // Compose into data struct and return
    return (inertialAngles){heading_degrees, roll_degrees, pitch_degrees, yaw_degrees};
}

inertialAccel imu_getAccel() {
    // Same process as getting Euler data
    uint8_t accel_data[6];
    int timeout0 = i2c_write_timeout_us(IMU_I2C, CHIP_REGISTER, &ACCEL_BEGIN_REGISTER, 1, true, 10000);
    int timeout1 = i2c_read_timeout_us(IMU_I2C, CHIP_REGISTER, accel_data, 6, false, 10000);
    if (timeout0 == PICO_ERROR_GENERIC || timeout0 == PICO_ERROR_TIMEOUT || timeout1 == PICO_ERROR_GENERIC || timeout1 == PICO_ERROR_TIMEOUT) {
        setIMUSafe(false);
        return (inertialAccel){0};
    }
    int16_t x = (accel_data[1] << 8) | accel_data[0];
    int16_t y = (accel_data[3] << 8) | accel_data[2];
    int16_t z = (accel_data[5] << 8) | accel_data[4];

    float accelX = x / 100.00;
    float accelY = y / 100.00;
    float accelZ = z / 100.00;
    return (inertialAccel){accelX, accelY, accelZ};
}

bool imu_changeMode(uint8_t mode) {
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
