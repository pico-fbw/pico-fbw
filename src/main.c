#include <stdio.h>
#include "pico/stdlib.h"

#include "io/servo.h"
#include "io/imu.h"
#include "modes/modes.h"
#include "config.h"

int main() {
    // Save time of 850ms after boot for later
    absolute_time_t imu_safe = make_timeout_time_ms(850);
    // Initialize power LED
    #ifndef PICO_DEFAULT_LED_PIN
      	#warning No default LED pin found. Power LED functionality may be impacted.
    #endif
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    // Set up I/O --
    // Initialize serial port over USB for debugging
    // TODO: Do NOT keep this in the final build! USB has a high overhead and I'm not exactly tethering my plane to a USB cable...
    stdio_init_all();

    // Set up PWM inputs
    // IF I HAD THEM

    // Set up servo/PWM outputs
    #ifdef DUAL_AIL
		servo_enable(SERVO_AIL_L_PIN);
		servo_enable(SERVO_AIL_R_PIN);
    #else
      	servo_enable(SERVO_AIL_PIN);
    #endif
		servo_enable(SERVO_ELEV_PIN);
		servo_enable(SERVO_RUD_PIN);

    // Wait before initializing IMU to give it time to boot if we haven't reached enough time yet
    sleep_until(imu_safe);
    // Initialize and configure IMU unit
    if (imu_init() == 0) {
        if (imu_configure() == 0) {
            // If IMU passes init and configure tests, set IMU data as safe to use
            setIMUSafe(true);
            // Set into normal mode
            setMode(1);
            // Enter main program loop
            while (true) {
                
            }
        } else {
            return 1;
        }
    } else {
        return 1;
    }

    // How did we get here?
    return 0;
}