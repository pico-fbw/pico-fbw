/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/int.h"

#include "io/esc.h"
#include "io/receiver.h"
#include "io/servo.h"

#include "sys/configuration.h"
#include "sys/control.h"

#include "direct.h"

void direct_update() {
    float ail = receiver_get((u32)config.pins[PINS_INPUT_AIL], RECEIVER_MODE_DEGREE);
    float ele = receiver_get((u32)config.pins[PINS_INPUT_ELE], RECEIVER_MODE_DEGREE);
    switch ((ControlMode)config.general[GENERAL_CONTROL_MODE]) {
        case CTRLMODE_3AXIS_ATHR:
        case CTRLMODE_3AXIS:
            servo_set((u32)config.pins[PINS_SERVO_RUD], receiver_get((u32)config.pins[PINS_INPUT_RUD], RECEIVER_MODE_DEGREE));
            /* fall through */
        case CTRLMODE_2AXIS_ATHR:
        case CTRLMODE_2AXIS:
            servo_set((u32)config.pins[PINS_SERVO_AIL], ail);
            servo_set((u32)config.pins[PINS_SERVO_ELE], ele);
            break;
        case CTRLMODE_FLYINGWING_ATHR:
        case CTRLMODE_FLYINGWING:
            servo_set((u32)config.pins[PINS_SERVO_AIL], control_mix_elevon(LEFT, ail, ele));
            servo_set((u32)config.pins[PINS_SERVO_ELE], control_mix_elevon(RIGHT, ail, ele));
            break;
    }
    if (receiver_has_athr())
        esc_set((u32)config.pins[PINS_ESC_THROTTLE],
                receiver_get((u32)config.pins[PINS_INPUT_THROTTLE], RECEIVER_MODE_PERCENT));
}
