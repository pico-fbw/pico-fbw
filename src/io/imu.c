#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

#include "imu.h"
#include "../config.h"

int imu_init() {
    // Check if the IMU is NOT enabled, if so, immediately throw an error (preprocessor error as well)
    #ifndef IMU_ENABLE
        #warning IMU features are disabled, was this intentional?
        return 1;
    #endif
    // Check if the BNO055 unit is defined, if so,
    #ifdef IMU_BNO055
		// Check for default i2c constants
		#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
			#warning No I2C defaults found, functionality may be impacted.
		#endif
		// Now, initialize i2c on default pins (typically 4 and 5)
		i2c_init(i2c_default, CHIP_FREQ_KHZ * 1000);
		gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
		gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
		gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
		gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
		bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
		// Query the ID register for expected values to confirm identity/check comms
		uint8_t id;
		int result = i2c_write_timeout_us(i2c_default, CHIP_REGISTER, &ID_REGISTER, 1, true, 1000000);
		if (result == PICO_ERROR_GENERIC) {
			return PICO_ERROR_GENERIC;
		} else if (result == PICO_ERROR_TIMEOUT) {
			return PICO_ERROR_TIMEOUT;
		} else {
				result = i2c_read_timeout_us(i2c_default, CHIP_REGISTER, &id, 1, false, 1000000);
				sleep_us(100);
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
		// If no BNO055 is defined, check if IMU is enabled, if so, stop compilation b/c that is an invalid combination (but IMU not enabled with no IMU selected is fine)
		#ifdef IMU_ENABLE
			#error No IMU module was defined. Please define an IMU module to use or disable IMU functionality.
		#endif
    #endif
}

int imu_configure() {
    // Use internal oscillator
    imu_write(SYS_REGISTER, 0x40);
    // Reset all interrupt status bits
    imu_write(SYS_REGISTER, 0x01);
    // Set normal power mode
    imu_write(PWR_MODE_REGISTER, PWR_MODE_NORMAL);
    sleep_ms(50);
    // Set default axis configuration
    imu_write(AXIS_MAP_CONF_REGISTER, 0x24);
    // Set default axis signs
    imu_write(AXIS_MAP_SIGN_REGISTER, 0x00);
    // Set units to mg (milli-Gs)
    imu_write(0x3B, 0b00000001);
    sleep_ms(30);
    // Set operation to NDOF
    imu_changeMode(MODE_NDOF);
}

struct inertialAngles imu_getInertialAngles() {
    // Get Euler data from IMU
    uint8_t euler_data[6];
    i2c_write_blocking(i2c_default, CHIP_REGISTER, &EULER_REGISTER, 1, true);
    i2c_read_blocking(i2c_default, CHIP_REGISTER, euler_data, 6, false);
    int16_t heading = (euler_data[1] << 8) | euler_data[0];
    int16_t roll = (euler_data[3] << 8) | euler_data[2];
    int16_t pitch = (euler_data[5] << 8) | euler_data[4];

    // Convert Euler data into angles
    float heading_degrees = (float)heading / 16.0;
    float roll_degrees = (float)roll / 16.0;
    float pitch_degrees = (float)pitch / 16.0;
    // Compose into data struct and return
    struct inertialAngles angles = {heading_degrees, roll_degrees, pitch_degrees};
    return angles;
}

int imu_changeMode(uint8_t mode) {
    imu_write(OPR_MODE_REGISTER, mode);
    sleep_ms(100);
    // Check to ensure mode has changed properly by reading it back
    uint8_t currentMode;
    i2c_write_blocking(i2c_default, CHIP_REGISTER, &OPR_MODE_REGISTER, 1, true);
    i2c_read_blocking(i2c_default, CHIP_REGISTER, &currentMode, 1, false);
    if (currentMode == mode) {
        return 0;
    } else {
        return 1;
    }
}

int imu_write(uint8_t address, uint8_t value) {
    uint8_t cmd[2] = {address, value};
    return i2c_write_blocking(i2c_default, CHIP_REGISTER, cmd, 2, true);
}