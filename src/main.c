/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "pico/config.h"
#include "pico/platform.h"
#include "pico/stdio.h"
#include "pico/time.h"
#include "pico/types.h"

#include "hardware/watchdog.h"
#ifdef RASPBERRYPI_PICO_W
    #include "pico/cyw43_arch.h"
#endif

#include "io/api.h"
#include "io/error.h"
#include "io/esc.h"
#include "io/flash.h"
#include "io/gps.h"
#include "io/imu.h"
#include "io/platform.h"
#include "io/pwm.h"
#include "io/servo.h"
#include "modes/modes.h"
#include "sys/config.h"
#include "sys/info.h"
#include "sys/switch.h"
#include "wifly/wifly.h"

int main() {
    stdio_init_all();
    sleep_ms(BOOT_WAIT_MS);
    #if defined(RASPBERRYPI_PICO)
        printf("\nhello and welcome to pico-fbw v%s!\n", PICO_FBW_VERSION);
    #elif defined(RASPBERRYPI_PICO_W)
        printf("\nhello and welcome to pico(w)-fbw v%s!\n", PICO_FBW_VERSION);
        printf("[boot] initializing cyw43 architecture\n");
        cyw43_arch_init();
    #endif
    platform_boot_begin();

    // Check for first boot
    if (flash_readFloat(FLOAT_SECTOR_BOOT, 0) != FLAG_BOOT) {
        printf("[boot] boot flag not found! initializing flash...\n");
        flash_reset();
        float boot[FLOAT_SECTOR_SIZE] = {FLAG_BOOT};
        flash_writeFloat(FLOAT_SECTOR_BOOT, boot);
        printf("[boot] writing default config values...\n");
        config_load(DEFAULT_VALUES);
        config_save();
        printf("[boot] done! rebooting now...\n");
        platform_reboot(REBOOT_FAST); // Reboot is done to ensure flash is okay; any problems with the flash will simply cause a bootloop
        while (true);
    } else {
        printf("[boot] boot flag ok\n");
    }

    // Check version
    printf("[boot] checking for updates\n");
    int versionCheck = info_checkVersion(flash_readString(STRING_SECTOR_VERSION));
    if (versionCheck < 0) {
        if (versionCheck < -2) {
            error_throw(ERROR_GENERAL, ERROR_LEVEL_FATAL, 250, 0, true, "Failed to run update checker!");
        } else {
            printf("[boot] performing a system update from v%s to v%s, please wait...\n", (flash_readString(STRING_SECTOR_VERSION) == NULL) ? "0.0.0" : flash_readString(STRING_SECTOR_VERSION), PICO_FBW_VERSION);
            // << Insert system update code here, if applicable >>
            // Update flash with new version
            flash_writeString(STRING_SECTOR_VERSION, PICO_FBW_VERSION);
            printf("[boot] no tasks to be performed\n");
        }
    } else {
        printf("[boot] no updates required\n");
    }

    // Load config
    printf("[boot] loading config\n");
    if (!config_load(FROM_FLASH)) error_throw(ERROR_GENERAL, ERROR_LEVEL_FATAL, 250, 0, true, "Failed to load config!");

    // PWM (in)
    uint num_pins = 5; // Maximum amount is 5 pins, may be overridden
    uint pins[num_pins];
    float deviations[num_pins];
    pwm_getPins(pins, &num_pins, deviations);
    if (config.general.skipCalibration) {
        if (config.debug.debug_fbw) printf("[boot] skipping PWM calibration, PWM will be disabled!\n");
    } else {
        if (config.debug.debug_fbw) printf("[boot] enabling PWM\n");
        pwm_enable(pins, num_pins);
        if (config.debug.debug_fbw) printf("[boot] validating PWM calibration\n");
        int calibrationResult = pwm_isCalibrated();
        switch (calibrationResult) {
            case -2:
                if (config.debug.debug_fbw) printf("[boot] PWM calibration was completed for a different control mode!\n");
            case -1:
                if (config.debug.debug_fbw) printf("[boot] PWM calibration not found!\n");
                sleep_ms(2000); // Wait a few moments for tx/rx to set itself up
                if (config.debug.debug_fbw) printf("[boot] calibrating now...do not touch the transmitter!\n");
                if (!pwm_calibrate(pins, num_pins, deviations, 2000, 2, 3) || pwm_isCalibrated() != 0) {
                    error_throw(ERROR_PWM, ERROR_LEVEL_FATAL, 500, 0, true, "PWM calibration failed!");
                } else {
                    if (config.debug.debug_fbw) printf("[boot] calibration successful!\n");
                }
                break;
        }
    }

    // Servos
    if (config.debug.debug_fbw) printf("[boot] enabling servos\n");
    uint num_servos = 3;
    uint servos[num_servos];
    servo_getPins(servos, &num_servos);
    for (uint8_t s = 0; s < num_servos; s++) {
        if (servo_enable(servos[s]) != 0) {
            if (config.debug.debug_fbw) printf("[boot] failed to initialize servo %d\n", s);
            error_throw(ERROR_PWM, ERROR_LEVEL_FATAL, 800, 0, false, "Failed to initialize a servo!");
        }
    }
    const uint16_t degrees[] = DEFAULT_SERVO_TEST;
    servo_test(servos, num_servos, degrees, NUM_DEFAULT_SERVO_TEST, DEFAULT_SERVO_TEST_PAUSE_MS);

    // ESC
    if (config.general.controlMode == CTRLMODE_3AXIS_ATHR || config.general.controlMode == CTRLMODE_FLYINGWING_ATHR) {
        if (config.debug.debug_fbw) printf("[boot] enabling ESC\n");
        if (esc_enable(config.pins1.escThrottle) != 0) error_throw(ERROR_PWM, ERROR_LEVEL_FATAL, 800, 0, false, "Failed to initialize the ESC!");
    }

    // IMU
    if (config.sensors.imuModel == IMU_MODEL_BNO055) {
        while (time_us_64() < (850 * 1000)); // BNO055 requires at least 850ms to ready up
    }
    if (config.general.skipCalibration) {
        if (config.debug.debug_fbw) printf("[boot] skipping IMU calibration, IMU will be disabled!\n");
    } else {
        if (config.debug.debug_fbw) printf("[boot] initializing IMU\n");
        if (imu_init() == 0) {
            if (imu_configure()) {
                if (config.debug.debug_fbw) printf("[boot] IMU ok\n");
                setIMUSafe(true);
                if (config.debug.debug_fbw) printf("[boot] checking for IMU calibration\n");
                if (!imu_isCalibrated()) {
                    if (config.debug.debug_fbw) printf("[boot] IMU calibration not found! waiting a bit to begin...\n");
                    sleep_ms(2000);
                    if (!imu_calibrate()) error_throw(ERROR_IMU, ERROR_LEVEL_FATAL, 1000, 0, true, "IMU calibration failed!");
                }
                if (config.debug.debug_fbw) printf("[boot] IMU axis calibration ok\n");
            } else {
                error_throw(ERROR_IMU, ERROR_LEVEL_WARN, 1000, 0, false, "IMU configuration failed!");
            }
        } else {
            error_throw(ERROR_IMU, ERROR_LEVEL_ERR, 1000, 0, false, "IMU not found!");
        }
    }

    // GPS
    if (config.sensors.gpsEnabled) {
        while (time_us_64() < (1000 * 1000));
        if (config.debug.debug_fbw) printf("[boot] initializing GPS\n");
        if (gps_init()) {
            if (config.debug.debug_fbw) printf("[boot] GPS ok\n");
            // We don't set the GPS safe just yet, communications have been established but we are still unsure if the data is okay
            error_throw(ERROR_GPS, ERROR_LEVEL_STATUS, 1000, 0, false, ""); // Show that GPS does not have a signal yet
        } else {
            error_throw(ERROR_GPS, ERROR_LEVEL_ERR, 2000, 0, false, "GPS initalization failed!");
        }
    }

    // Wi-Fly
    #ifdef RASPBERRYPI_PICO_W
        if (config.debug.debug_fbw) printf("[boot] initializing Wi-Fly\n");
        wifly_init();
    #endif

    // Watchdog
    watchdog_enable(config.debug.watchdog_timeout_ms, true);
    if (platform_boot_type() == BOOT_WATCHDOG) {
        error_throw(ERROR_GENERAL, ERROR_LEVEL_ERR, 500, 150, true, "Watchdog rebooted!");
        if (config.debug.debug_fbw) printf("Please report this error! Only direct mode is available until the next reboot.\n");
        platform_boot_complete();
        toMode(MODE_DIRECT);
        while (true) {
            modeRuntime();
            watchdog_update();
        }
    }

    // Main program loop:
    platform_boot_complete();
    if (config.debug.debug_fbw) printf("[boot] bootup complete!\n");
    while (true) {
        // Update the mode switch's position, run the current mode's code, respond to any new API calls, and update the watchdog
        float switchPos = pwm_read(config.pins0.inputSwitch, PWM_MODE_DEG);
        switch (config.general.switchType) {
            case SWITCH_TYPE_2_POS:
                if (switchPos < 90) {
                    switch_update(SWITCH_POSITION_LOW);
                } else {
                    switch_update(SWITCH_POSITION_HIGH);
                }
                break;
            case SWITCH_TYPE_3_POS:
                if (switchPos < 85) {
                    switch_update(SWITCH_POSITION_LOW);
                } else if (switchPos > 95) {
                    switch_update(SWITCH_POSITION_HIGH);
                } else {
                    switch_update(SWITCH_POSITION_MID);
                }
                break;
        }
        modeRuntime();
        if (config.general.apiEnabled) api_poll();
        watchdog_update();
    }

    return 0; // How did we get here?
}
