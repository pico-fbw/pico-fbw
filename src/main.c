#include <stdio.h>
#include "pico/stdlib.h"

#include "io/servo.h"
#include "io/pwm.h"
#include "io/imu.h"
#include "io/led.h"
#include "modes/modes.h"
#include "modes/direct.h"
#include "modes/normal.h"
#include "config.h"

int main() {
    // Save time of 850ms after boot for later
    absolute_time_t imu_safe = make_timeout_time_ms(850);
    // Initialize power LED
    led_init();

    // Set up I/O --
    // Initialize serial port over USB for debugging
    // TODO: Do NOT keep this in the final build! USB has a high overhead and I'm not exactly tethering my plane to a USB cable...
    stdio_init_all();

    // Set up PWM inputs
    uint pin_list[] = {INPUT_AIL_PIN};
    pwm_enable(pin_list, 1);

    // Set up and test PWM (servo) outputs
    const uint8_t servos[] = {SERVO_AIL_PIN, SERVO_ELEV_PIN, SERVO_RUD_PIN};
    const uint16_t degrees[] = {135, 45};
    for (int s = 0; s < 3; s++) {
        servo_enable(servos[s]);
    }
    for (int d = 0; d < 2; d++) {
        for (int s = 0; s < 3; s++) {
            servo_set(servos[s], degrees[d]);
        }
        // This delay is purposely not long enough for the servos to reach that final degree value, the test is just to see if they move in both directions
        sleep_ms(300);
    }

    // Wait before initializing IMU to give it time to boot just in case we haven't reached enough time yet
    sleep_until(imu_safe);
    // Initialize and configure IMU unit
    if (imu_init() == 0) {
        if (imu_configure() == 0) {
            // If IMU passes init and configure tests, set IMU data as safe to use
            setIMUSafe(true);
            // Set into normal mode
            setMode(1);
        // If IMU does not pass both tests,
        } else {
            // Slow blink LED: IMU init error
            led_blink(1000);
        }
    } else {
        led_blink(1000);
    }

    // Enter main program loop
    while (true) {
        if (getMode() == 0) {
            // Direct mode
            mode_direct();
        } else if (getMode() == 1) {
            // Normal mode
            mode_normal();
        }
    }

    // How did we get here?
    return 0;
}