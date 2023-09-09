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
#include "io/switch.h"
#include "wifly/wifly.h"

#include "lib/info.h"

#include "modes/modes.h"

#include "config.h"
#include "validator.h"

int main() {
    // Crucial items--initialize comms, api, architecture, io, and LED
    stdio_init_all();
    #ifdef FBW_DEBUG
        sleep_ms(BOOTUP_WAIT_TIME_MS); // Wait for serial to begin
    #endif
    #if defined(RASPBERRYPI_PICO)
        FBW_DEBUG_printf("\nhello and welcome to pico-fbw v%s!\n", PICO_FBW_VERSION);
    #elif defined(RASPBERRYPI_PICO_W)
        FBW_DEBUG_printf("\nhello and welcome to pico(w)-fbw v%s!\n", PICO_FBW_VERSION);
        FBW_DEBUG_printf("[driver] initializing cyw43 architecture with predefined country 0x%04X\n", WIFLY_NETWORK_COUNTRY);
        cyw43_arch_init_with_country(WIFLY_NETWORK_COUNTRY);
    #endif
    platform_boot_begin();

    // API
    #ifdef API_ENABLED
        printf("[api] enabling api v%s\n", PICO_FBW_API_VERSION);
        error_throw(ERROR_NONE, ERROR_LEVEL_NONE, 1000, 100, false, "");
        api_init_blocking();
        error_clear(ERROR_NONE, true);
    #endif

    // Check for first boot
    FBW_DEBUG_printf("[driver] starting bootup process\n");
    if (flash_read(FLASH_SECTOR_BOOT, 0) != FLAG_BOOT) {
        FBW_DEBUG_printf("[boot] boot flag not found! assuming first boot, initializing flash\n");
        flash_reset();
        float boot[CONFIG_SECTOR_SIZE] = {FLAG_BOOT};
        flash_write(FLASH_SECTOR_BOOT, boot);
        FBW_DEBUG_printf("[boot] boot data written! rebooting now...\n");
        // Reboot is to ensure flash is okay; any problems with the flash will simply cause a bootloop before getting to anything important
        watchdog_enable(1, 1);
        while (true);
    } else {
        FBW_DEBUG_printf("[boot] boot flag ok\n");
    }

    // PWM (in)
    #if defined(CONTROL_3AXIS)
        #ifdef ATHR_ENABLED
            uint pins[] = {INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_RUD_PIN, INPUT_SW_PIN, INPUT_THR_PIN};
            uint num_pins = 5;
            float deviations[] = {90.0f, 90.0f, 90.0f, 0.0f, 0.0f}; // We expect all controls to be centered except switch and throttle
        #else
            uint pins[] = {INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_RUD_PIN, INPUT_SW_PIN};
            uint num_pins = 4;
            float deviations[] = {90.0f, 90.0f, 90.0f, 0.0f};
        #endif
    #elif defined(CONTROL_FLYINGWING)
        #ifdef ATHR_ENABLED
            uint pins[] = {INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_SW_PIN, INPUT_THR_PIN};
            uint num_pins = 4;
            float deviations[] = {90.0f, 90.0f, 0.0f, 0.0f};
        #else
            uint pins[] = {INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_SW_PIN};
            uint num_pins = 3;
            float deviations[] = {90.0f, 90.0f, 0.0f};
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
            // Both -1 and -3 will result in a calibration
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
    #if defined(CONTROL_3AXIS)
        const uint8_t servos[] = {SERVO_AIL_PIN, SERVO_ELEV_PIN, SERVO_RUD_PIN};
        uint num_servos = 3;
    #elif defined(CONTROL_FLYINGWING)
        const uint8_t servos[] = {SERVO_ELEVON_L_PIN, SERVO_ELEVON_R_PIN};
        uint num_servos = 2;
    #endif
    for (uint8_t s = 0; s < num_servos; s++) {
        if (servo_enable(servos[s]) != 0) {
            FBW_DEBUG_printf("[boot] failed to initialize servo %d)\n", s);
            error_throw(ERROR_PWM, ERROR_LEVEL_FATAL, 800, 0, false, "Failed to initialize a servo!");
        }
    }
    FBW_DEBUG_printf("[boot] testing servos, watch for movement\n");
    const uint8_t degrees[] = {105, 75};
    for (uint8_t d = 0; d < 2; d++) {
        for (uint8_t s = 0; s < num_servos; s++) {
            servo_set(servos[s], degrees[d]);
        }
        sleep_ms(50);
    }
    for (uint8_t s = 0; s < num_servos; s++) {
        servo_set(servos[s], 90);
    }
    #ifdef ATHR_ENABLED
        FBW_DEBUG_printf("[boot] enabling ESC\n");
        if (esc_enable(ESC_THR_PIN) != 0) {
            error_throw(ERROR_PWM, ERROR_LEVEL_FATAL, 800, 0, false, "Failed to initialize the ESC!");
        }
    #endif

    // IMU
    #ifdef IMU_BNO055
        while (time_us_32() < (850 * 1000)); // BNO055 requires at least 850ms to ready up
    #endif
    FBW_DEBUG_printf("[boot] initializing IMU\n");
    if (imu_init() == 0) {
        if (imu_configure()) {
            FBW_DEBUG_printf("[boot] IMU ok\n");
            setIMUSafe(true);
            FBW_DEBUG_printf("[boot] checking for IMU axis calibration\n");
            if (!imu_isCalibrated()) {
                FBW_DEBUG_printf("[boot] IMU axis calibration not found! waiting a bit to begin...\n");
                sleep_ms(3000);
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
        while (time_us_32() < (500 * 1000)); // Give GPS time (at least 500ms after power-up) to initialize
        FBW_DEBUG_printf("[boot] initializing GPS\n");
        if (gps_init()) {
            FBW_DEBUG_printf("[boot] GPS ok\n");
            // We don't set the GPS safe just yet, communications have been established but we are still unsure if the data is okay
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
    FBW_DEBUG_printf("[boot] bootup complete! entering main program loop...\n");
    while (true) {
        // Update the mode switch's position
        float switchPos = pwm_read(INPUT_SW_PIN, PWM_MODE_DEG);
        #ifdef SWITCH_2_POS
            if (switchPos < 90) {
                // Lower pos
                updateSwitch(DIRECT);
            } else {
                // Upper pos
                updateSwitch(NORMAL);
            }
        #endif // SWITCH_2_POS
        #ifdef SWITCH_3_POS
            if (switchPos < 85) {
                // Lower pos
                updateSwitch(DIRECT);
            } else if (switchPos > 95) {
                // Upper pos
                updateSwitch(AUTO);
            } else {
                // Middle pos
                updateSwitch(NORMAL);
            }
        #endif // SWITCH_3_POS

        // Run the current mode's runtime
        modeRuntime();

        // Check for new API calls if applicable
        #ifdef API_ENABLED
            api_poll();
        #endif
    }

    return 0; // How did we get here?
}
