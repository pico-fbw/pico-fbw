#include <stdio.h>
#include "pico/stdlib.h"

#include "io/servo.h"
#include "io/imu.h"
#include "config.h"

int main() {
    // Save time of 850ms after boot (for later)
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

    // Set the pins for servo/PWM outputs
    #ifdef DUAL_AIL
      servo_enable(SERVO_AIL_L_PIN);
      servo_enable(SERVO_AIL_R_PIN);
    #else
      servo_enable(SERVO_AIL_PIN);
    #endif
    servo_enable(SERVO_ELEV_PIN);
    servo_enable(SERVO_RUD_PIN);

    // Wait before initializing IMU to give it time to boot
    sleep_until(imu_safe);
    // Initialize and configure IMU unit
    if (imu_init() == 0) {
        if (imu_configure() == 0) {
            // Enter main program loop
            while (true) {
                inertialAngles angles = imu_getInertialAngles();
                printf("Heading: %.2f, Roll: %.2f, Pitch: %.2f\n", angles.heading, angles.roll, angles.pitch);
                sleep_ms(100);
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