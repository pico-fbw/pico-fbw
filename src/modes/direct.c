/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/types.h"

#include "ctrl/control.h"
#include "io/esc.h"
#include "io/receiver.h"
#include "io/servo.h"
#include "sys/configuration.h"

#include "direct.h"

void direct_update() {
    f32 ail = control_apply_expo(receiver_get((i16)config.pins[PINS_INPUT_AIL], RECEIVER_MODE_DEGREE));
    f32 ele = control_apply_expo(receiver_get((i16)config.pins[PINS_INPUT_ELE], RECEIVER_MODE_DEGREE));
    switch ((ControlMode)config.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS:
            f32 rud = control_apply_expo(receiver_get((i16)config.pins[PINS_INPUT_RUD], RECEIVER_MODE_DEGREE));
            servo_set((i16)config.pins[PINS_SERVO_RUD], rud);
            /* fall through */
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_2AXIS:
            servo_set((i16)config.pins[PINS_SERVO_AIL], ail);
            servo_set((i16)config.pins[PINS_SERVO_ELE], ele);
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            servo_set((i16)config.pins[PINS_SERVO_AIL], control_mix_elevon(ELEVON_LEFT, ail, ele));
            servo_set((i16)config.pins[PINS_SERVO_ELE], control_mix_elevon(ELEVON_RIGHT, ail, ele));
            break;
    }
    if (receiver_has_athr()) {
        f32 esc = receiver_get((i16)config.pins[PINS_INPUT_THROTTLE], RECEIVER_MODE_PERCENT);
        esc_set((i16)config.pins[PINS_ESC_THROTTLE], esc);
    }
}
