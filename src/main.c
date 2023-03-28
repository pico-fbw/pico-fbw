#include <stdio.h>
#include "pico/stdlib.h"

#include "io/servo.h"
#include "io/imu.h"
#include "config.h"

int main() {
    // Initialize serial port over USB for debugging
    // TODO: Do NOT keep this in the final build! USB has a high overhead and I'm not exactly tethering my plane to a USB cable...
    stdio_init_all();
    sleep_ms(5000);

    // Initialize power LED
    #ifndef PICO_DEFAULT_LED_PIN
      #warning No default LED pin found. Power LED functionality may be impacted.
    #endif
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    // Set up I/O
    
    // Set the pins for servo/PWM outputs
    #ifdef DUAL_AIL
      servoEnable(SERVO_AIL_L_PIN);
      servoEnable(SERVO_AIL_R_PIN);
    #else
      servoEnable(SERVO_AIL_PIN);
    #endif
    servoEnable(SERVO_ELEV_PIN);
    servoEnable(SERVO_RUD_PIN);

    printf("imuInit has returned: %d\n", imuInit());
    printf("imuConfigure has returned: %d\n", imuConfigure());

    // Note for me tomorrow--I'm dead inside so I can't code anymore but here have this fun video that should help
    // <3 past you :)
    // https://www.youtube.com/watch?v=092xFEmAS98

    return 0;
}