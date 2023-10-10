/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
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

#include "io/esc.h"
#include "io/flash.h"
#include "io/gps.h"
#include "io/imu.h"
#include "io/platform.h"
#include "io/pwm.h"
#include "io/servo.h"
#include "modes/modes.h"
#include "sys/api/api.h"
#include "sys/config.h"
#include "sys/info.h"
#include "sys/log.h"
#include "sys/switch.h"
#include "wifly/wifly.h"

int main() {
    platform_boot_begin();
    if (stdio_init_all()) sleep_ms(BOOT_WAIT_MS);
    #if defined(RASPBERRYPI_PICO)
        printf("\nhello and welcome to pico-fbw v%s!\n", PICO_FBW_VERSION);
    #elif defined(RASPBERRYPI_PICO_W)
        printf("\nhello and welcome to pico(w)-fbw v%s!\n", PICO_FBW_VERSION);
        platform_boot_setProgress(10, "Initializing CYW43 architecture");
        cyw43_arch_init();
    #endif
    log_init();

    platform_boot_setProgress(20, "Caching flash");
    printf("[boot] cached %d bytes of flash into RAM\n", flash_cache());

    // First boot (with pico-fbw) actions
    if (flash_readFloat(FLOAT_SECTOR_BOOT, 0) != FLAG_BOOT) {
        platform_boot_setProgress(50, "Formatting flash");
        flash_reset();
        float boot[FLOAT_SECTOR_SIZE] = {FLAG_BOOT};
        flash_writeFloat(FLOAT_SECTOR_BOOT, boot, true);
        platform_boot_setProgress(80, "Writing default config values");
        config_load(DEFAULT_VALUES);
        config_save(false);
        platform_boot_setProgress(100, "Rebooting");
        platform_reboot(REBOOT_FAST); // Reboot is done to ensure flash is okay; any problems with the flash will simply cause a bootloop
    } else {
        platform_boot_setProgress(25, "Checking boot flag");
        printf("[boot] boot flag ok\n");
    }

    // Check version
    platform_boot_setProgress(30, "Checking for updates");
    int versionCheck = info_checkVersion(flash_readString(STRING_SECTOR_VERSION));
    if (versionCheck < 0) {
        if (versionCheck < -2) {
            log_message(FATAL, "Failed to run update checker!", 250, 0, true);
        } else {
            printf("[boot] performing a system update from v%s to v%s, please wait...\n", (flash_readString(STRING_SECTOR_VERSION) == NULL) ? "0.0.0" : flash_readString(STRING_SECTOR_VERSION), PICO_FBW_VERSION);
            // << Insert system update code here, if applicable >>
            // Update flash with new version
            flash_writeString(STRING_SECTOR_VERSION, PICO_FBW_VERSION, true);
            printf("[boot] no tasks to be performed\n");
        }
    } else {
        printf("[boot] no updates required\n");
    }

    // Load config
    platform_boot_setProgress(35, "Loading config");
    if (!config_load(FROM_FLASH)) log_message(FATAL, "Failed to load config!", 250, 0, true);

    // PWM (in)
    uint num_pins = 5; // Maximum amount is 5 pins, may be overridden
    uint pins[num_pins];
    float deviations[num_pins];
    pwm_getPins(pins, &num_pins, deviations);
    if (config.general.skipCalibration) {
        if (config.debug.debug_fbw) printf("[boot] skipping PWM calibration, PWM will be disabled!\n");
    } else {
        platform_boot_setProgress(40, "Enabling PWM");
        pwm_enable(pins, num_pins);
        if (config.debug.debug_fbw) printf("[boot] validating PWM calibration\n");
        int calibrationResult = pwm_isCalibrated();
        switch (calibrationResult) {
            case PWMCALIBRATION_INVALID:
                if (config.debug.debug_fbw) printf("[boot] PWM calibration was completed for a different control mode!\n");
            case PWMCALIBRATION_INCOMPLETE:
                if (config.debug.debug_fbw) printf("[boot] PWM calibration not found!\n");
                if (config.debug.debug_fbw) printf("[boot] calibrating now...do not touch the transmitter!\n");
                if (!pwm_calibrate(pins, num_pins, deviations, 2000, 2, 3) || pwm_isCalibrated() != 0) {
                    log_message(FATAL, "PWM calibration failed!", 500, 0, true);
                } else {
                    if (config.debug.debug_fbw) printf("[boot] calibration successful!\n");
                }
                break;
        }
    }

    // Servos
    platform_boot_setProgress(50, "Enabling servos");
    uint num_servos = 3;
    uint servos[num_servos];
    servo_getPins(servos, &num_servos);
    for (uint8_t s = 0; s < num_servos; s++) {
        if (servo_enable(servos[s]) != 0) {
            if (config.debug.debug_fbw) printf("[boot] failed to initialize servo %d\n", s);
            log_message(FATAL, "Failed to initialize a servo!", 800, 0, false);
        }
    }
    const uint16_t degrees[] = DEFAULT_SERVO_TEST;
    servo_test(servos, num_servos, degrees, NUM_DEFAULT_SERVO_TEST, DEFAULT_SERVO_TEST_PAUSE_MS);

    // ESC
    if (config.general.controlMode == CTRLMODE_3AXIS_ATHR || config.general.controlMode == CTRLMODE_FLYINGWING_ATHR) {
        platform_boot_setProgress(60, "Enabling ESC");
        if (esc_enable(config.pins1.escThrottle) != 0) log_message(FATAL, "Failed to initialize an ESC!", 800, 0, false);
    }

    // IMU
    if (config.sensors.imuModel == IMU_MODEL_BNO055) {
        while (time_us_64() < (850 * 1000)); // BNO055 requires at least 850ms to ready up
    }
    if (config.general.skipCalibration) {
        if (config.debug.debug_fbw) printf("[boot] skipping IMU calibration, IMU will be disabled!\n");
    } else {
        platform_boot_setProgress(65, "Initializing IMU");
        if (imu_init() == 0) {
            if (imu_configure()) {
                if (config.debug.debug_fbw) printf("[boot] IMU ok\n");
                setIMUSafe(true);
                if (config.debug.debug_fbw) printf("[boot] checking for IMU calibration\n");
                if (!imu_isCalibrated()) {
                    if (config.debug.debug_fbw) printf("[boot] IMU calibration not found!\n");
                    sleep_ms(2000);
                    if (!imu_calibrate()) log_message(FATAL, "IMU calibration failed!", 1000, 0, true);
                }
                if (config.debug.debug_fbw) printf("[boot] IMU axis calibration ok\n");
            } else {
                log_message(WARNING, "IMU configuration failed!", 1000, 0, false);
            }
        } else {
            log_message(ERROR, "IMU not found!", 1000, 0, false);
        }
    }

    // GPS
    if (config.sensors.gpsEnabled) {
        while (time_us_64() < (1000 * 1000));
        platform_boot_setProgress(80, "Initializing GPS");
        if (gps_init()) {
            if (config.debug.debug_fbw) printf("[boot] GPS ok\n");
            // We don't set the GPS safe just yet, communications have been established but we are still unsure if the data is okay
            log_message(LOG, "GPS has no signal.", 2000, 0, false);
        } else {
            log_message(ERROR, "GPS not found!", 2000, 0, false);
        }
    }

    // Wi-Fly
    #ifdef RASPBERRYPI_PICO_W
        platform_boot_setProgress(90, "Initializing Wi-Fly");
        wifly_init();
    #endif

    // Watchdog
    platform_boot_setProgress(95, "Enabling watchdog");
    // watchdog_enable(config.debug.watchdog_timeout_ms, true);
    if (platform_boot_type() == BOOT_WATCHDOG) {
        log_message(ERROR, "Watchdog rebooted!", 500, 150, true);
        if (config.debug.debug_fbw) printf("Please report this error! Only direct mode is available until the next reboot.\n");
        platform_boot_complete();
        toMode(MODE_DIRECT);
        while (true) {
            modeRuntime();
            watchdog_update();
        }
    }

    platform_boot_setProgress(100, "Done!");
    platform_boot_complete();
    // Main program loop:
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
