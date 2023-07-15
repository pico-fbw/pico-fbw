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
#include "io/flash.h"
#include "io/gps.h"
#include "io/imu.h"
#include "io/led.h"
#include "io/pwm.h"
#include "io/servo.h"
#include "io/switch.h"
#ifdef WIFLY_ENABLED
    #include "io/wifly/wifly.h"
#endif

#include "modes/modes.h"

#include "config.h"
#include "version.h"

int main() {
    // Before-bootup crucial items (timestamp bootup, initialize comms, api, architecture, io, and LED)
    absolute_time_t imu_safe = make_timeout_time_ms(850);
    stdio_init_all();
    #ifdef FBW_DEBUG
        sleep_ms(BOOTUP_WAIT_TIME_MS); // Wait for serial to begin
    #endif
    #ifdef RASPBERRYPI_PICO
        FBW_DEBUG_printf("\nhello and welcome to pico-fbw v%s!\n", PICO_FBW_VERSION);
    #endif
    #ifdef RASPBERRYPI_PICO_W
        FBW_DEBUG_printf("\nhello and welcome to pico-fbw(w) v%s!\n", PICO_FBW_VERSION);
        #ifdef WIFLY_ENABLED
            FBW_DEBUG_printf("[driver] initializing cyw43 architecture with predefined country 0x%04X\n", WIFLY_NETWORK_COUNTRY);
            cyw43_arch_init_with_country(WIFLY_NETWORK_COUNTRY);
        #else
            FBW_DEBUG_printf("[driver] initializing cyw43 architecture without country\n");
            cyw43_arch_init();
        #endif
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
        FBW_DEBUG_printf("[boot] boot data written successfully! rebooting now...\n");
        watchdog_enable(1, 1);
        while (true);
    } else {
        FBW_DEBUG_printf("[boot] boot flag ok\n");
    }
    
    // PWM IN
    FBW_DEBUG_printf("[boot] checking for PWM IN calibration\n");
    if (!pwm_checkCalibration()) {
        FBW_DEBUG_printf("[boot] PWM IN calibration not found! waiting for tx/rx...\n");
        sleep_ms(3000); // Wait a few moments for tx/rx to set itself up
        FBW_DEBUG_printf("[boot] calibrating now...do not touch the transmitter!\n");
        // Calibrate PWM
        if (!pwm_calibrate(90.0f, 2000, 5, 5) || !pwm_checkCalibration()) {
            FBW_DEBUG_printf("[boot] FATAL: [FBW-500] PWM IN calibration failed\n");
            led_blink(500, 0);
            while (true);
        }
        FBW_DEBUG_printf("[boot] calibration successful, rebooting now\n");
        watchdog_enable(1, 1);
    }
    // Check to make sure PWM calibration values seem alright, this is mainly to protect extremely high calibration values from being used, such as if a channel was accidentally unplugged during calubration
    if (pwm_getCalibrationValue(0) > MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(0) < -MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(1) > MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(1) < -MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(2) > MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(2) < -MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(3) > MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(3) < -MAX_CALIBRATION_OFFSET) {
        FBW_DEBUG_printf("[boot] FATAL: [FBW-500] PWM IN calibration values were too high!\n");
        FBW_DEBUG_printf("Try again, and if this continues, consider changing the MAX_CALIBRATION_OFFSET in the configuration file.\n");
        led_blink(500, 0);
        while (true);
    }
    FBW_DEBUG_printf("[boot] PWM IN calibration ok, enabling\n");
    uint pin_list[] = {INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_RUD_PIN, MODE_SWITCH_PIN};
    pwm_enable(pin_list, 4);

    // Servos
    FBW_DEBUG_printf("[boot] enabling servos\n");
    const uint8_t servos[] = {SERVO_AIL_PIN, SERVO_ELEV_PIN, SERVO_RUD_PIN};
    for (uint8_t s = 0; s < 3; s++) {
        if (servo_enable(servos[s]) != 0) {
            FBW_DEBUG_printf("[boot] FATAL: [FBW-800] failed to initialize servo %d)\n", s);
            // TODO: when I get back home add a GIF of FBW-800 to the wiki
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
        sleep_ms(100);
    }
    for (uint8_t s = 0; s < 3; s++) {
        servo_set(servos[s], 90);
    }

    // IMU
    sleep_until(imu_safe);
    FBW_DEBUG_printf("[boot] initializing IMU\n");
    if (imu_init() == 0) {
        if (imu_configure()) {
            FBW_DEBUG_printf("[boot] IMU ok\n");
            setIMUSafe(true);
        } else {
            FBW_DEBUG_printf("[boot] WARNING: [FBW-1000] IMU configuration failed!\n");
            led_blink(1000, 0);
        }
    } else {
        FBW_DEBUG_printf("[boot] WARNING: [FBW-1000] IMU not found!\n");
        led_blink(1000, 0);
    }

    // GPS and Wi-Fly
    #ifdef WIFLY_ENABLED
        FBW_DEBUG_printf("[boot] initializing GPS\n");
        if (gps_init()) {
            FBW_DEBUG_printf("[boot] GPS ok\n");
            setGPSSafe(true);
        } else {
            FBW_DEBUG_printf("[boot] WARNING: [FBW-1000] GPS not found!\n");
            led_blink(2000, 0);
        }
        FBW_DEBUG_printf("[boot] initializing Wi-Fly\n");
        wifly_init();
    #endif

    // Main program loop: update the mode switch's position, then run the current mode's runtime
    FBW_DEBUG_printf("[boot] bootup complete! entering main program loop...\n");
    while (true) {
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
        modeRuntime();
        #ifdef API_ENABLED
            api_poll();
        #endif
    }

    return 0; // How did we get here?
}
