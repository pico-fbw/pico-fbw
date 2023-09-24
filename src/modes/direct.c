/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include "pico/types.h"

#include "../io/esc.h"
#include "../io/servo.h"
#include "../io/pwm.h"

#include "../sys/config.h"

#include "direct.h"

/**
 * Quick note for future me/any other developers (if there ever are lol):
 * This mode is as low-latency as I can think to make it--the only way (I believe) to make it faster is to
 * reassign the GPIO pins being used for input to DIO instead of PIO and assign them back when switching modes, which
 * gave a basically unnoticeable decrease in latency and it's much more complicated, so I didn't bother to implement it.
*/

void mode_direct() {
    switch (config.general.controlMode) {
        case CTRLMODE_3AXIS_ATHR:
            esc_set(config.pins1.escThrottle, pwm_read(config.pins1.inputThrottle, PWM_MODE_ESC));
        case CTRLMODE_3AXIS:
            servo_set(config.pins0.servoAil, pwm_read(config.pins0.inputAil, PWM_MODE_DEG));
            servo_set(config.pins0.servoElev, pwm_read(config.pins0.inputElev, PWM_MODE_DEG));
            servo_set(config.pins0.servoRud, pwm_read(config.pins0.inputRud, PWM_MODE_DEG));
            break;
        case CTRLMODE_FLYINGWING_ATHR:
            esc_set(config.pins1.escThrottle, pwm_read(config.pins1.inputThrottle, PWM_MODE_ESC));
        case CTRLMODE_FLYINGWING:
            servo_set(config.pins1.servoElevonL, pwm_read(config.pins0.inputAil, PWM_MODE_DEG));
            servo_set(config.pins1.servoElevonR, pwm_read(config.pins0.inputElev, PWM_MODE_DEG));
            break;
    }
}
