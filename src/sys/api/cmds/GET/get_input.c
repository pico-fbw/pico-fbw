/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>

#include "../../../../io/flash.h"
#include "../../../../io/pwm.h"

#include "get_input.h"

int api_get_input(const char *cmd, const char *args) {
    printf("{\"ail\":%f,\"elev\":%f,\"rud\":%f,\"thr\":%f,\"switch\":%f}\n", pwm_read(flash.pins[PINS_INPUT_AIL], PWM_MODE_DEG),
           pwm_read(flash.pins[PINS_INPUT_ELEV], PWM_MODE_DEG), pwm_read(flash.pins[PINS_INPUT_RUD], PWM_MODE_DEG),
           pwm_read(flash.pins[PINS_INPUT_THROTTLE], PWM_MODE_ESC), pwm_read(flash.pins[PINS_INPUT_SWITCH], PWM_MODE_DEG));
    return -1;
}
