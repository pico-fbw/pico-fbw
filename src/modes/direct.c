/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include "pico/types.h"

#include "../io/esc.h"
#include "../io/servo.h"
#include "../io/pwm.h"

#include "../config.h"

#include "direct.h"

/**
 * Quick note for future me/any other developers:
 * This mode is as low-latency as I can think to make it--the only way (I believe) to make it faster is to
 * reassign the GPIO pins being used for input to DIO instead of PIO and assign them back when switching modes, which
 * gave a basically unnoticeable decrease in latency and it's much more complicated, so I didn't bother to implement it.
*/

void mode_direct() {
    #if defined(CONTROL_3AXIS)
        servo_set(SERVO_AIL_PIN, pwm_read(INPUT_AIL_PIN, PWM_MODE_DEG));
        servo_set(SERVO_ELEV_PIN, pwm_read(INPUT_ELEV_PIN, PWM_MODE_DEG));
        servo_set(SERVO_RUD_PIN, pwm_read(INPUT_RUD_PIN, PWM_MODE_DEG));
    #elif defined(CONTROL_FLYINGWING)
        servo_set(SERVO_ELEVON_L_PIN, pwm_read(INPUT_AIL_PIN, PWM_MODE_DEG));
        servo_set(SERVO_ELEVON_R_PIN, pwm_read(INPUT_ELEV_PIN, PWM_MODE_DEG));
    #endif
    #ifdef ATHR_ENABLED
        esc_set(ESC_THR_PIN, pwm_read(INPUT_THR_PIN, PWM_MODE_ESC));
    #endif
}
