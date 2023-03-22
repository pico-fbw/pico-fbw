#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"

#include "io/servo.h"
#include "config.h"

int main() { 
    // Initialize power LED
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1); 
    // Initialize serial port over USB for debugging (may not keep in final build)
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

    bool clockwise = true;
    uint angle = 0;

    while (true) {
        printf("%u\n",angle);
        servo_set_position(SERVO_AIL_PIN, angle);
        sleep_ms(5);
        if (clockwise) {
            angle++;
        } else {
            angle--;
        }
        if (angle == 0 || angle == 180) {
            clockwise = !clockwise;
            sleep_ms(2500);
        }
    }
    return 0;
}