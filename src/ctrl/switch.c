/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include "platform/types.h"

#include "ctrl/aircraft.h"
#include "io/receiver.h"
#include "sys/configuration.h"
#include "sys/flightplan.h"

#include "switch.h"

typedef enum SwitchPosition {
    SWITCH_POSITION_LOW,
    SWITCH_POSITION_MID,
    SWITCH_POSITION_HIGH,
} SwitchPosition;

static SwitchPosition lastPos;

/**
 * @return position of the mode switch based on the current switch type and "angle"
 */
static SwitchPosition deg_to_pos(f32 deg) {
    switch ((SwitchType)config.general[GENERAL_SWITCH_TYPE]) {
        case SWITCH_TYPE_2_POS:
            if (deg < 90) {
                return SWITCH_POSITION_LOW;
            } else {
                return SWITCH_POSITION_HIGH;
            }
        default:
        case SWITCH_TYPE_3_POS:
            if (deg < 45) {
                return SWITCH_POSITION_LOW;
            } else if (deg > 135) {
                return SWITCH_POSITION_HIGH;
            } else {
                return SWITCH_POSITION_MID;
            }
    }
}

void switch_update() {
    SwitchPosition pos = deg_to_pos(receiver_get((u32)config.pins[PINS_INPUT_SWITCH], RECEIVER_MODE_DEGREE));
    // The mode will only be changed when the user moves the switch;
    // the system's mode changes can persist and won't instantly be overrided by the switch
    if (lastPos == pos) {
        return;
    }
    switch (pos) {
        case SWITCH_POSITION_LOW:
            aircraft.change_to(MODE_DIRECT);
            break;
        case SWITCH_POSITION_MID:
            aircraft.change_to(MODE_NORMAL);
            break;
        case SWITCH_POSITION_HIGH:
            switch ((SwitchType)config.general[GENERAL_SWITCH_TYPE]) {
                case SWITCH_TYPE_2_POS:
                    // For 2-position switches, auto-select auto or normal mode based on if a flight plan is present
                    if (flightplan_get()) {
                        aircraft.change_to(MODE_AUTO);
                    } else {
                        aircraft.change_to(MODE_NORMAL);
                    }
                    break;
                case SWITCH_TYPE_3_POS:
                    aircraft.change_to(MODE_AUTO);
                    break;
            }
            break;
    }
    lastPos = pos;
}
