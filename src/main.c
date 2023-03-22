#include <stdio.h>
#include "pico/stdlib.h"

#include "io/servo.h"

const uint SERVO_PIN = 15;

void main() {  
    stdio_init_all();
    
    // Init power LED
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);

    servo_enable(SERVO_PIN);

    bool clockwise = true;
    uint angle = 0;

    while (true) {
        printf("%u\n",angle);
        servo_set_position(SERVO_PIN, angle);
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
}