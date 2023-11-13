/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdint.h>

#include "../io/flash.h"

#include "../modes/modes.h"

#include "switch.h"

static SwitchPosition lastPos;

void switch_update(SwitchPosition pos) {
    // The mode will only be changed when the user moves the switch; the system's mode changes can persist and won't instantly be overrided by the switch
    if (lastPos != pos) {
        switch (pos) {
            case SWITCH_POSITION_LOW:
                aircraft.changeTo(MODE_DIRECT);
                break;
            case SWITCH_POSITION_MID:
                aircraft.changeTo(MODE_NORMAL);
                break;
            case SWITCH_POSITION_HIGH:
                switch ((SwitchType)flash.general[GENERAL_SWITCH_TYPE]) {
                    case SWITCH_TYPE_2_POS:
                        aircraft.changeTo(MODE_NORMAL);
                        break;
                    case SWITCH_TYPE_3_POS:
                        aircraft.changeTo(MODE_AUTO);
                        break;
                }
                break;
        }
        lastPos = pos;
    }
}
