/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include "pico/stdio.h"
#include "pico/time.h"
#include "hardware/watchdog.h"
#ifdef RASPBERRYPI_PICO_W
    #include "pico/cyw43_arch.h"
#endif

#include "io/flash.h"
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
    // Before-bootup crucial items (timestamp bootup, initialize comms, architecture, and io)
    absolute_time_t imu_safe = make_timeout_time_ms(850);
    stdio_init_all();
    #ifdef FBW_DEBUG
        sleep_ms(800); // Wait for serial to begin
    #endif  
    FBW_DEBUG_printf("hello and welcome to pico-fbw v%s!\n\n", PICO_FBW_VERSION);
    #ifdef RASPBERRYPI_PICO_W
        #ifdef WIFLY_ENABLED
            FBW_DEBUG_printf("[init] initializing cyw43 architecture with predefined country %04X\n", WIFLY_NETWORK_COUNTRY);
            cyw43_arch_init_with_country(WIFLY_NETWORK_COUNTRY);
        #else
            FBW_DEBUG_printf("[init] initializing cyw43 architecture without country\n");
            cyw43_arch_init();
        #endif
    #endif
    FBW_DEBUG_printf("[init] initializing status LED\n");
    led_init();

    // Check for first boot
    FBW_DEBUG_printf("[init] initializing bootup process\n\n");
    if (flash_read(3, 0) != FBW_BOOT) {
        FBW_DEBUG_printf("[boot] boot flag not found! assuming first boot, initializing flash\n");
        flash_reset();
        float boot[CONFIG_SECTOR_SIZE] = {FBW_BOOT};
        flash_write(3, boot);
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
            led_blink(500);
            while (true);
        }
        FBW_DEBUG_printf("[boot] calibration successful, rebooting now\n");
        watchdog_enable(1, 1);
    }
    // Check to make sure PWM calibration values seem alright, this is mainly to protect extremely high calibration values from being used, such as if a channel was accidentally unplugged during calubration
    if (pwm_getCalibrationValue(0) > MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(0) < -MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(1) > MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(1) < -MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(2) > MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(2) < -MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(3) > MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(3) < -MAX_CALIBRATION_OFFSET) {
        FBW_DEBUG_printf("[boot] FATAL: [FBW-500] PWM IN calibration values were too high!\n");
        FBW_DEBUG_printf("Try again, and if this continues, consider changing the MAX_CALIBRATION_OFFSET in the configuration file.\n");
        led_blink(500);
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
            led_blink(800);
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
    FBW_DEBUG_printf("[boot] servos ok\n");

    // IMU
    sleep_until(imu_safe);
    FBW_DEBUG_printf("[boot] initializing IMU\n");
    if (imu_init() == 0) {
        if (imu_configure()) {
            FBW_DEBUG_printf("[boot] IMU ok\n");
            setIMUSafe(true);
        } else {
            FBW_DEBUG_printf("[boot] WARNING: [FBW-1000] IMU configuration failed!\n");
            led_blink(1000);
        }
    } else {
        FBW_DEBUG_printf("[boot] WARNING: [FBW-1000] IMU not found\n");
        led_blink(1000);
    }

    // Wi-Fly
    #ifdef WIFLY_ENABLED
        FBW_DEBUG_printf("\n[boot] initializing Wi-Fly\n");
        wifly_init();
    #endif

    // Main program loop
    FBW_DEBUG_printf("\n[boot] bootup complete! entering main program loop...\n\n");
    while (true) {
        #ifdef SWITCH_2_POS
            if (pwm_readDeg(3) < 90) {
                // Lower pos
                switchPos(DIRECT);
            } else {
                // Upper pos
                switchPos(NORMAL);
            }
        #endif // switch_2_pos
        #ifdef SWITCH_3_POS
            if (pwm_readDeg(3) < 85) {
                // Lower pos
                switchPos(DIRECT);
            } else if (pwm_readDeg(3) > 95) {
                // Upper pos
                switchPos(AUTO);
            } else {
                // Middle pos
                switchPos(NORMAL);
            }
        #endif // switch_3_pos
    }

    return 0; // How did we get here?
}
