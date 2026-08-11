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
    f32 ail = control_apply_expo(receiver_get((i16)config.pins.inputAil, RECEIVER_MODE_DEGREE));
    f32 ele = control_apply_expo(receiver_get((i16)config.pins.inputEle, RECEIVER_MODE_DEGREE));
    switch ((ControlMode)config.general.controlMode) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS: {
            f32 rud = control_apply_expo(receiver_get((i16)config.pins.inputRud, RECEIVER_MODE_DEGREE));
            servo_set((i16)config.pins.servoRud, rud);
            /* fall through */
        }
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_2AXIS:
            servo_set((i16)config.pins.servoAil, ail);
            servo_set((i16)config.pins.servoEle, ele);
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            servo_set((i16)config.pins.servoAil, control_mix_elevon(ELEVON_LEFT, ail, ele));
            servo_set((i16)config.pins.servoEle, control_mix_elevon(ELEVON_RIGHT, ail, ele));
            break;
    }
    if (receiver_has_athr()) {
        f32 esc = receiver_get((i16)config.pins.inputThrottle, RECEIVER_MODE_PERCENT);
        esc_set((i16)config.pins.escThrottle, esc);
    }
}
