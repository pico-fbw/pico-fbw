/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
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

#include "validator.h"

#define BOOTUP_WAIT_MS 900 // The amount of time (in ms) to wait for any possible serial connections to be established before booting.

int main() {
    stdio_init_all();
    sleep_ms(BOOTUP_WAIT_MS);
    #if defined(RASPBERRYPI_PICO)
        FBW_DEBUG_printf("\nhello and welcome to pico-fbw v%s!\n", PICO_FBW_VERSION);
    #elif defined(RASPBERRYPI_PICO_W)
        FBW_DEBUG_printf("\nhello and welcome to pico(w)-fbw v%s!\n", PICO_FBW_VERSION);
        FBW_DEBUG_printf("[boot] initializing cyw43 architecture\n");
        cyw43_arch_init();
    #endif
    platform_boot_begin();

    // Check for first boot
    if (flash_readFloat(FLOAT_SECTOR_BOOT, 0) != FLAG_BOOT) {
        FBW_DEBUG_printf("[boot] boot flag not found! initializing flash...\n");
        flash_reset();
        // TODO: initialize default config here
        float boot[FLOAT_SECTOR_SIZE] = {FLAG_BOOT};
        flash_writeFloat(FLOAT_SECTOR_BOOT, boot);
        FBW_DEBUG_printf("[boot] done! rebooting now...\n");
        // Reboot is done to ensure flash is okay; any problems with the flash will simply cause a bootloop
        watchdog_enable(1, false);
        while (true);
    } else {
        FBW_DEBUG_printf("[boot] boot flag ok\n");
    }

    // Check version
    FBW_DEBUG_printf("[boot] checking for updates\n");
    int versionCheck = info_checkVersion(flash_readString(STRING_SECTOR_VERSION));
    if (versionCheck < 0) {
        if (versionCheck < -2) {
            platform_boot_complete();
            error_throw(ERROR_GENERAL, ERROR_LEVEL_FATAL, 250, 0, true, "Failed to run update checker!");
        } else {
            FBW_DEBUG_printf("[boot] performing a system update from v%s to v%s, please wait...\n", (flash_readString(STRING_SECTOR_VERSION) == NULL) ? "0.0.0" : flash_readString(STRING_SECTOR_VERSION), PICO_FBW_VERSION);
            // << Insert system update code here, if applicable >>
            // Update flash with new version
            flash_writeString(STRING_SECTOR_VERSION, PICO_FBW_VERSION);
            FBW_DEBUG_printf("[boot] no tasks to be performed\n");
        }
    } else {
        FBW_DEBUG_printf("[boot] no updates required\n");
    }

    // Load config
    FBW_DEBUG_printf("[boot] loading config\n");
    config_load();

    // PWM (in)
    #if defined(CONTROL_3AXIS)
        #ifdef ATHR_ENABLED
            const uint pins[] = {INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_RUD_PIN, INPUT_SW_PIN, INPUT_THR_PIN};
            const uint num_pins = 5;
            const float deviations[] = {90.0f, 90.0f, 90.0f, 0.0f, 0.0f}; // We expect all controls to be centered except switch and throttle
        #else
            const uint pins[] = {INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_RUD_PIN, INPUT_SW_PIN};
            const uint num_pins = 4;
            const float deviations[] = {90.0f, 90.0f, 90.0f, 0.0f};
        #endif
    #elif defined(CONTROL_FLYINGWING)
        #ifdef ATHR_ENABLED
            const uint pins[] = {INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_SW_PIN, INPUT_THR_PIN};
            const uint num_pins = 4;
            const float deviations[] = {90.0f, 90.0f, 0.0f, 0.0f};
        #else
            const uint pins[] = {INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_SW_PIN};
            const uint num_pins = 3;
            const float deviations[] = {90.0f, 90.0f, 0.0f};
        #endif
    #endif
    FBW_DEBUG_printf("[boot] enabling PWM\n");
    pwm_enable(pins, num_pins);
    FBW_DEBUG_printf("[boot] validating PWM\n");
    int calibrationResult = pwm_isCalibrated();
    switch (calibrationResult) {
        case -2:
            FBW_DEBUG_printf("[boot] PWM calibration was completed for a different control mode!\n");
        case -1:
            FBW_DEBUG_printf("[boot] PWM calibration not found!\n");
            platform_boot_complete();
            sleep_ms(2000); // Wait a few moments for tx/rx to set itself up
            FBW_DEBUG_printf("[boot] calibrating now...do not touch the transmitter!\n");
            if (!pwm_calibrate(pins, num_pins, deviations, 2000, 2, 3) || pwm_isCalibrated() != 0) {
                error_throw(ERROR_PWM, ERROR_LEVEL_FATAL, 500, 0, true, "PWM calibration failed!");
            } else {
                FBW_DEBUG_printf("[boot] calibration successful!\n");
            }
            break;
    }

    // Servos/ESC (PWM out)
    FBW_DEBUG_printf("[boot] enabling servos\n");
    const uint servos[] = SERVO_PINS;
    for (uint8_t s = 0; s < NUM_SERVOS; s++) {
        if (servo_enable(servos[s]) != 0) {
            FBW_DEBUG_printf("[boot] failed to initialize servo %d\n", s);
            error_throw(ERROR_PWM, ERROR_LEVEL_FATAL, 800, 0, false, "Failed to initialize a servo!");
        }
    }
    const uint16_t degrees[] = DEFAULT_SERVO_TEST;
    servo_test(servos, NUM_SERVOS, degrees, NUM_DEFAULT_SERVO_TEST, DEFAULT_SERVO_TEST_PAUSE_MS);
    #ifdef ATHR_ENABLED
        FBW_DEBUG_printf("[boot] enabling ESC\n");
        if (esc_enable(ESC_THR_PIN) != 0) {
            error_throw(ERROR_PWM, ERROR_LEVEL_FATAL, 800, 0, false, "Failed to initialize the ESC!");
        }
    #endif

    // IMU
    #ifdef IMU_BNO055
        while (time_us_64() < (850 * 1000)); // BNO055 requires at least 850ms to ready up
    #endif
    FBW_DEBUG_printf("[boot] initializing IMU\n");
    if (imu_init() == 0) {
        if (imu_configure()) {
            FBW_DEBUG_printf("[boot] IMU ok\n");
            setIMUSafe(true);
            FBW_DEBUG_printf("[boot] checking for IMU calibration\n");
            if (!imu_isCalibrated()) {
                FBW_DEBUG_printf("[boot] IMU calibration not found! waiting a bit to begin...\n");
                platform_boot_complete();
                sleep_ms(2000);
                if (!imu_calibrate()) {
                    error_throw(ERROR_IMU, ERROR_LEVEL_FATAL, 1000, 0, true, "IMU calibration failed!");
                }
            }
            FBW_DEBUG_printf("[boot] IMU axis calibration ok\n");
        } else {
            error_throw(ERROR_IMU, ERROR_LEVEL_WARN, 1000, 0, false, "IMU configuration failed!");
        }
    } else {
        error_throw(ERROR_IMU, ERROR_LEVEL_ERR, 1000, 0, false, "IMU not found!");
    }

    // GPS
    #ifdef GPS_ENABLED
        while (time_us_64() < (500 * 1000)); // Give GPS time (at least 500ms after power-up) to initialize
        FBW_DEBUG_printf("[boot] initializing GPS\n");
        if (gps_init()) {
            FBW_DEBUG_printf("[boot] GPS ok\n");
            // We don't set the GPS safe just yet, communications have been established but we are still unsure if the data is okay
            error_throw(ERROR_GPS, ERROR_LEVEL_STATUS, 1000, 0, false, ""); // Show that GPS does not have a signal yet
        } else {
            error_throw(ERROR_GPS, ERROR_LEVEL_ERR, 1000, 0, false, "GPS initalization failed!");
        }
    #endif

    // Wi-Fly
    #ifdef WIFLY_ENABLED
        FBW_DEBUG_printf("[boot] initializing Wi-Fly\n");
        wifly_init();
    #endif

    // Main program loop:
    platform_boot_complete();
    FBW_DEBUG_printf("[boot] bootup complete!\n");
    while (true) {
        // Update the mode switch's position
        float switchPos = pwm_read(INPUT_SW_PIN, PWM_MODE_DEG);
        switch (configGeneral.switchType) {
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

        // Run the current mode's code
        modeRuntime();

        // Respond to any new API calls
        #ifdef API_ENABLED
            api_poll();
        #endif
    }

    return 0; // How did we get here?
}
