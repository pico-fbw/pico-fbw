#include <stdio.h>
#include "pico/stdlib.h"
#ifdef RASPBERRYPI_PICO_W
    #include "pico/cyw43_arch.h"
#endif

#include "io/servo.h"
#include "io/pwm.h"
#include "io/imu.h"
#include "io/led.h"
#ifdef WIFLY_ENABLED
    #include "io/wifly/wifly.h"
#endif    
#include "modes/modes.h"
#include "config.h"

int main() {
    // Save time of 850ms after boot for later
    absolute_time_t imu_safe = make_timeout_time_ms(850);
    // Initialize cyw43 arch if we are on the Pico W
    #ifdef RASPBERRYPI_PICO_W
        #ifdef WIFLY_ENABLED
            cyw43_arch_init_with_country(WIFLY_NETWORK_COUNTRY);
        #else
            cyw43_arch_init();
        #endif
    #endif
    // Initialize power LED
    led_init();
    // Initialize any linked IO types
    stdio_init_all();
    
    // Set up PWM inputs
    uint pin_list[] = {INPUT_AIL_PIN, INPUT_ELEV_PIN, INPUT_RUD_PIN, MODE_SWITCH_PIN};
    pwm_enable(pin_list, 4);
    // Set up and test PWM (servo) outputs
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

    // If PWM has not been previously calibrated (likely first boot),
    if (!pwm_checkCalibration()) {
        // Wait a few s for tx/rx to set itself up
        sleep_ms(3000);
        // Calibrate PWM (offset of 90 degrees, 2000 samples with 5ms delay and 5 times sample, this should take about 60s)
        // If either the calibration itself or the final check fails, throw error FBW-500
        if (!pwm_calibrate(90.0f, 2000, 5, 5) || !pwm_checkCalibration()) {
            led_blink(500);
            while (true);
        }
    }

    // Check to make sure PWM calibration values seem alright, otherwise stop execution like above because bad things could happen...
    // This is mainly to protect extremely high calibration values from being used, such as if a channel was accidentally unplugged during calubration
    if (pwm_getCalibrationValue(0) > MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(0) < -MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(1) > MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(1) < -MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(2) > MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(2) < -MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(3) > MAX_CALIBRATION_OFFSET || pwm_getCalibrationValue(3) < -MAX_CALIBRATION_OFFSET) {
        led_blink(500);
        while (true);
    }

    // Wait before initializing IMU to give it time to boot just in case we haven't reached enough time yet
    sleep_until(imu_safe);
    // Initialize and configure IMU unit
    if (imu_init() == 0) {
        if (imu_configure()) {
            // If IMU passes init and configure tests, set IMU data as safe to use
            setIMUSafe(true);
        // If IMU does not pass both tests,
        } else {
            // Slow blink LED: IMU init error
            led_blink(1000);
        }
    } else {
        led_blink(1000);
    }

    // If Wi-Fly is enabled, we initialize it here
    #ifdef WIFLY_ENABLED
        wifly_init();
    #endif

    // Enter main program loop
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
