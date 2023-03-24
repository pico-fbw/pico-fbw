#include <stdio.h>
#include "pico/stdlib.h"

#include "io/servo.h"
#include "config.h"

int main() { 
    // Initialize power LED
    #ifndef PICO_DEFAULT_LED_PIN;
    #warning No default LED pin found. Power LED functionality may be impacted.
    #endif
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    // Initialize serial port over USB for debugging
    // TODO: Do NOT keep this in the final build! USB has a high overhead and I'm not exactly tethering my plane to a USB cable...
    stdio_init_all();

    // Set up I/O
    // TODO: Set the pins for analog input (servos and 1 extra from rx)
    // possibly use example code here: https://github.com/raspberrypi/pico-examples/blob/master/pwm/measure_duty_cycle/measure_duty_cycle.c
    
    // Set the pins for servos/PWM outputs
    #ifdef DUAL_AIL
      servo_enable(SERVO_AIL_L_PIN);
      servo_enable(SERVO_AIL_R_PIN);
    #else
      servo_enable(SERVO_AIL_PIN);
    #endif
    servo_enable(SERVO_ELEV_PIN);
    servo_enable(SERVO_RUD_PIN);

    // Test code for direct law (passthrough mode)


    return 0;
}