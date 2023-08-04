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

#include "hardware/watchdog.h"
#ifdef RASPBERRYPI_PICO_W
    #include "pico/cyw43_arch.h"
#endif

#include "io/api.h"
#include "io/binary.h"
#include "io/flash.h"
#include "io/gps.h"
#include "io/imu.h"
#include "io/led.h"
#include "io/pwm.h"
#include "io/servo.h"
#include "io/switch.h"
#include "io/wifly/wifly.h"

#include "modes/modes.h"

#include "config.h"
#include "validator.h"
#include "version.h"

int main() {
    // Crucial items--initialize comms, api, architecture, io, and LED
    stdio_init_all();
    #ifdef FBW_DEBUG
        sleep_ms(BOOTUP_WAIT_TIME_MS); // Wait for serial to begin
    #endif
    #ifdef RASPBERRYPI_PICO
        FBW_DEBUG_printf("\nhello and welcome to pico-fbw v%s!\n", PICO_FBW_VERSION);
    #endif
    #ifdef RASPBERRYPI_PICO_W
        FBW_DEBUG_printf("\nhello and welcome to pico(w)-fbw v%s!\n", PICO_FBW_VERSION);
        FBW_DEBUG_printf("[driver] initializing cyw43 architecture with predefined country 0x%04X\n", WIFLY_NETWORK_COUNTRY);
        cyw43_arch_init_with_country(WIFLY_NETWORK_COUNTRY);
    #endif
    led_init();

    // API
    #ifdef API_ENABLED
        printf("[api] enabling api v%s\n", PICO_FBW_API_VERSION);
        led_blink(1000, 100);
        api_init_blocking();
        led_stop();
    #endif

    // Check for first boot
    FBW_DEBUG_printf("[driver] starting bootup process\n");
    if (flash_read(FLASH_SECTOR_BOOT, 0) != FBW_BOOT) {
        FBW_DEBUG_printf("[boot] boot flag not found! assuming first boot, initializing flash\n");
        flash_reset();
        float boot[CONFIG_SECTOR_SIZE] = {FBW_BOOT};
        flash_write(FLASH_SECTOR_BOOT, boot);
        FBW_DEBUG_printf("[boot] boot data written! rebooting now...\n");
        // Reboot is to ensure flash is okay; any problems with the flash will simply cause a bootloop before getting to anything important
        watchdog_enable(1, 1);
        while (true);
    } else {
        FBW_DEBUG_printf("[boot] boot flag ok\n");
    }

    // PWM
    FBW_DEBUG_printf("[boot] checking for PWM calibration\n");
    if (pwm_checkCalibration() != 0) {
        if (pwm_checkCalibration() == -1) {
            FBW_DEBUG_printf("[boot] PWM calibration not found! waiting for tx/rx...\n");
            sleep_ms(3000); // Wait a few moments for tx/rx to set itself up
            FBW_DEBUG_printf("[boot] calibrating now...do not touch the transmitter!\n");
            if (!pwm_calibrate(90.0f, 2000, 5, 5) || pwm_checkCalibration() != 0) {
                FBW_DEBUG_printf("[boot] FATAL: [FBW-500] PWM calibration failed!\n");
                led_blink(500, 0);
                while (true);
            } else {
                FBW_DEBUG_printf("[boot] calibration successful!\n");
            }
        } else if (pwm_checkCalibration() == -2) {
            FBW_DEBUG_printf("[boot] FATAL: [FBW-500] PWM calibration values were too high!\n");
            FBW_DEBUG_printf("Try again, and if this continues, consider changing the MAX_CALIBRATION_OFFSET in the configuration file.\n");
            led_blink(500, 0);
            while (true);
        }
    }
    FBW_DEBUG_printf("[boot] PWM calibration ok, enabling\n");
    uint pin_list[] = {INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_RUD_PIN, MODE_SWITCH_PIN};
    pwm_enable(pin_list, 4);

    // Servos
    FBW_DEBUG_printf("[boot] enabling servos\n");
    const uint8_t servos[] = {SERVO_AIL_PIN, SERVO_ELEV_PIN, SERVO_RUD_PIN};
    for (uint8_t s = 0; s < 3; s++) {
        if (servo_enable(servos[s]) != 0) {
            FBW_DEBUG_printf("[boot] FATAL: [FBW-800] failed to initialize servo %d)\n", s);
            led_blink(800, 0);
            while (true);
        }
    }
    FBW_DEBUG_printf("[boot] testing servos, watch for movement\n");
    const uint8_t degrees[] = {105, 75};
    for (uint8_t d = 0; d < 2; d++) {
        for (uint8_t s = 0; s < 3; s++) {
            servo_set(servos[s], degrees[d]);
        }
        sleep_ms(50);
    }
    for (uint8_t s = 0; s < 3; s++) {
        servo_set(servos[s], 90);
    }

    // GPS
    #ifdef GPS_ENABLED
        while (time_us_32() < (500 * 1000)); // Give GPS time (at least 500ms after power-up) to initialize
        FBW_DEBUG_printf("[boot] initializing GPS\n");
        if (gps_init()) {
            FBW_DEBUG_printf("[boot] GPS ok\n");
            // We don't set the GPS safe just yet, communications have been established but we are still unsure if the data is okay
        } else {
            FBW_DEBUG_printf("[boot] WARNING: [FBW-2000] GPS initalization failed!\n");
            led_blink(2000, 0);
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
            if (!imu_checkCalibration()) {
                FBW_DEBUG_printf("[boot] IMU axis calibration not found! waiting a bit to begin...\n");
                sleep_ms(3000);
                if (!imu_calibrate()) {
                    FBW_DEBUG_printf("[boot] FATAL: [FBW-500] IMU calibration failed!\n");
                    led_blink(500, 0);
                    while (true);
                }
            }
            FBW_DEBUG_printf("[boot] IMU axis calibration ok\n");
        } else {
            FBW_DEBUG_printf("[boot] WARNING: [FBW-1000] IMU configuration failed!\n");
            led_blink(1000, 0);
        }
    } else {
        FBW_DEBUG_printf("[boot] WARNING: [FBW-1000] IMU not found!\n");
        led_blink(1000, 0);
    }

    // Wi-Fly
    #ifdef WIFLY_ENABLED
        FBW_DEBUG_printf("[boot] initializing Wi-Fly\n");
        wifly_init();
    #endif

    // Main program loop:
    FBW_DEBUG_printf("[boot] bootup complete! entering main program loop...\n");
    while (true) {
        // Update the mode switch's position
        float switchPos = pwm_readDeg(3);
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

    declare_binary();
}
