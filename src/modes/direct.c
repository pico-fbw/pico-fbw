#include "pico/stdlib.h"

#include "../io/servo.h"
#include "../io/pwm.h"
#include "../config.h"

#include "direct.h"

float ail_deg, elev_deg, rud_deg;

void mode_direct() {
    // This mode is basically as low-latency as I can make it--the only way to make it faster is
    // to reassign the GPIO pins being used for input to DIO instead of PIO and assign them back when switching modes, which I sadly can't do in circumstance.
    ail_deg = pwm_readDeg(0);
    elev_deg = pwm_readDeg(1);
    rud_deg = pwm_readDeg(2);
    servo_set(SERVO_AIL_PIN, ail_deg);
    servo_set(SERVO_ELEV_PIN, elev_deg);
    servo_set(SERVO_RUD_PIN, rud_deg);
}