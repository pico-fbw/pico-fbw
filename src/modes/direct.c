#include "pico/stdlib.h"

#include "../io/servo.h"
#include "../io/pwm.h"
#include "../config.h"

#include "direct.h"

void mode_direct() {
    // This mode is basically as low-latency as I can make it--the only way to make it faster is
    // to reassign the GPIO pins being used for input to DIO instead of PIO and assign them back when switching modes, which
    // gives a negligable decrease in latency :(
    servo_set(SERVO_AIL_PIN, pwm_readDeg(0));
    servo_set(SERVO_ELEV_PIN, pwm_readDeg(1));
    servo_set(SERVO_RUD_PIN, pwm_readDeg(2));
}