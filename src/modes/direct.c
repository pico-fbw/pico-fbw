/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include "pico/types.h"

#include "../io/esc.h"
#include "../io/flash.h"
#include "../io/servo.h"
#include "../io/pwm.h"

#include "direct.h"

/**
 * Quick note for future me/any other developers (if there ever are lol):
 * This mode is as low-latency as I can think to make it--the only way (I believe) to make it faster is to
 * reassign the GPIO pins being used for input to DIO instead of PIO and assign them back when switching modes, which
 * gave a basically unnoticeable decrease in latency and it's much more complicated, so I didn't bother to implement it.
*/

void mode_direct() {
    switch ((ControlMode)flash.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
            esc_set((uint)flash.pins[PINS_ESC_THROTTLE], (uint16_t)pwm_read((uint)flash.pins[PINS_INPUT_THROTTLE], PWM_MODE_ESC));
        case CTRLMODE_3AXIS:
            servo_set((uint)flash.pins[PINS_SERVO_AIL], (uint16_t)pwm_read((uint)flash.pins[PINS_INPUT_AIL], PWM_MODE_DEG));
            servo_set((uint)flash.pins[PINS_SERVO_ELEV], (uint16_t)pwm_read((uint)flash.pins[PINS_INPUT_ELEV], PWM_MODE_DEG));
            servo_set((uint)flash.pins[PINS_SERVO_RUD], (uint16_t)pwm_read((uint)flash.pins[PINS_INPUT_RUD], PWM_MODE_DEG));
            break;
        case CTRLMODE_FLYINGWING_ATHR:
            esc_set((uint)flash.pins[PINS_ESC_THROTTLE], (uint16_t)pwm_read((uint)flash.pins[PINS_INPUT_THROTTLE], PWM_MODE_ESC));
        case CTRLMODE_FLYINGWING:
            // TODO: flying wing mixing here
            servo_set((uint)flash.pins[PINS_SERVO_ELEVON_L], (uint16_t)pwm_read((uint)flash.pins[PINS_INPUT_AIL], PWM_MODE_DEG));
            servo_set((uint)flash.pins[PINS_SERVO_ELEVON_R], (uint16_t)pwm_read((uint)flash.pins[PINS_INPUT_ELEV], PWM_MODE_DEG));
            break;
    }
}
