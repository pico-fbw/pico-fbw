#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#ifdef RASPBERRYPI_PICO_W
    #include "pico/cyw43_arch.h"
#endif

#include "io/servo.h"
#include "io/pwm.h"
#include "io/imu.h"
#include "io/led.h"
#include "io/flash.h"
#ifdef WIFLY_ENABLED
    #include "io/wifly/wifly.h"
#endif    
#include "modes/modes.h"
#include "config.h"
#include "version.h"

int main() {
    stdio_init_all();
    #if MISC_DEBUG
        sleep_ms(800);
    #endif  
    FBW_DEBUG_printf("Hello and welcome to pico-fbw!\n");
    // Save time of 850ms after boot for later
    absolute_time_t imu_safe = make_timeout_time_ms(850);
    #ifdef RASPBERRYPI_PICO_W
        #ifdef WIFLY_ENABLED
            FBW_DEBUG_printf("Initializing cyw43 architecture with predefined country %04X\n", WIFLY_NETWORK_COUNTRY);
            cyw43_arch_init_with_country(WIFLY_NETWORK_COUNTRY);
        #else
            FBW_DEBUG_printf("Initializing cyw43 architecture\n");
            cyw43_arch_init();
        #endif
    #endif
    FBW_DEBUG_printf("Initializing status LED\n");
    led_init();

    FBW_DEBUG_printf("Checking for boot flag...\n");
    if (flash_read(3, 1) != FBW_BOOT) {
        FBW_DEBUG_printf("Boot flag not found! Wiping flash data...\n");
        flash_reset();
        FBW_DEBUG_printf("Flash successfully wiped. Now writing boot data...\n");
        float boot[CONFIG_SECTOR_SIZE];
        boot[0] = PICO_FBW_VERSION;
        boot[1] = FBW_BOOT;
        flash_write(3, boot);
        FBW_DEBUG_printf("Boot data written successfully! Rebooting now...\n");
        watchdog_enable(1, 1);
        while (true);
    }
    FBW_DEBUG_printf("Boot flag found: %f\n", flash_read(3, 1));
    FBW_DEBUG_printf("Boot flag ok\n");
    FBW_DEBUG_printf("You are on firmware version %f\n\n", flash_read(3, 0));
    
    FBW_DEBUG_printf("Setting up PWM IN subsystem\n");
    uint pin_list[] = {INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_RUD_PIN, MODE_SWITCH_PIN};
    pwm_enable(pin_list, 4);
    FBW_DEBUG_printf("PWM IN ok\n");
    FBW_DEBUG_printf("Testing PWM OUT subsystem, watch for movement!\n");
    const uint8_t servos[] = {SERVO_AIL_PIN, SERVO_ELEV_PIN, SERVO_RUD_PIN};
    const uint8_t degrees[] = {105, 75};
    for (uint8_t s = 0; s < 3; s++) {
        servo_enable(servos[s]);
    }
    for (uint8_t d = 0; d < 2; d++) {
        for (uint8_t s = 0; s < 3; s++) {
            servo_set(servos[s], degrees[d]);
        }
        sleep_ms(100);
    }
    for (uint8_t s = 0; s < 3; s++) {
        servo_set(servos[s], 90);
    }
    FBW_DEBUG_printf("PWM OUT ok\n");

    FBW_DEBUG_printf("Checking for PWM IN calibration\n");
    if (!pwm_checkCalibration()) {
        FBW_DEBUG_printf("PWM IN calibration not found! Waiting...\n");
        // Wait a few moments for tx/rx to set itself up
        sleep_ms(3000);
        FBW_DEBUG_printf("Calibrating now...do not touch your transmitter!\n");
        // Calibrate PWM (offset of 90 degrees, 2000 samples with 5ms delay and 5 times sample, this should take about 60s)
        // If either the calibration itself or the final check fails, throw error FBW-500
        if (!pwm_calibrate(90.0f, 2000, 5, 5) || !pwm_checkCalibration()) {
            FBW_DEBUG_printf("FATAL: [FBW-500] PWM IN calibration failed\n");
            led_blink(500);
            while (true);
        }
    }

    // Check to make sure PWM calibration values seem alright, otherwise stop execution like above because bad things could happen...
    // This is mainly to protect extremely high calibration values from being used, such as if a channel was accidentally unplugged during calubration
    if (pwm_getCalibrationValue(0) > MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(0) < -MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(1) > MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(1) < -MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(2) > MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(2) < -MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(3) > MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(3) < -MAX_CALIBRATION_OFFSET) {
        FBW_DEBUG_printf("FATAL: [FBW-500] PWM IN calibration failed, values were too high!\n");
        FBW_DEBUG_printf("Try again, and if this happens continually, consider changing the MAX_CALIBRATION_OFFSET in the configuration file.\n");
        led_blink(500);
        while (true);
    }
    FBW_DEBUG_printf("PWM IN calibration ok\n\n");

    // Wait before initializing IMU to give it time to boot just in case we haven't reached enough time yet
    sleep_until(imu_safe);
    FBW_DEBUG_printf("Initializing IMU subsystem\n");
    if (imu_init() == 0) {
        if (imu_configure()) {
            // If IMU passes init and configure tests, set IMU data as safe to use
            FBW_DEBUG_printf("IMU ok\n");
            setIMUSafe(true);
        // If IMU does not pass both tests,
        } else {
            FBW_DEBUG_printf("WARNING: [FBW-1000] IMU configuration failed!\n");
            led_blink(1000);
        }
    } else {
        FBW_DEBUG_printf("WARNING: [FBW-1000] IMU not found\n");
        led_blink(1000);
    }

    #ifdef WIFLY_ENABLED
        FBW_DEBUG_printf("\nInitializing Wi-Fly subsystem\n");
        wifly_init();
    #endif

    FBW_DEBUG_printf("\nBootup complete! Entering main program loop...\n");
    while (true) {
        #ifdef SWITCH_2_POS
            if (pwm_readDeg(3) < 90) {
                // Lower pos
                mode(DIRECT);
            } else {
                // Upper pos
                mode(NORMAL);
            }
        #endif // switch_2_pos
        #ifdef SWITCH_3_POS
            if (pwm_readDeg(3) < 85) {
                // Lower pos
                mode(DIRECT);
            } else if (pwm_readDeg(3) > 95) {
                // Upper pos
                mode(AUTO);
            } else {
                // Middle pos
                mode(NORMAL);
            }
        #endif // switch_3_pos
    }

    // How did we get here?
    return 0;
}
