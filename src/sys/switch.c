/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdint.h>

#include "../modes/modes.h"

#include "config.h"

#include "switch.h"

static SwitchPosition lastPos;

void switch_update(SwitchPosition pos) {
    // The mode will only be changed when the user moves the switch; the system's mode changes can persist and won't instantly be overrided by the switch
    if (lastPos != pos) {
        switch (pos) {
            case SWITCH_POSITION_LOW:
                toMode(MODE_DIRECT);
                break;
            case SWITCH_POSITION_MID:
                toMode(MODE_NORMAL);
                break;
            case SWITCH_POSITION_HIGH:
                switch (config.general.switchType) {
                    case SWITCH_TYPE_2_POS:
                        toMode(MODE_NORMAL);
                        break;
                    case SWITCH_TYPE_3_POS:
                        toMode(MODE_AUTO);
                        break;
                }
                break;
        }
        lastPos = pos;
    }
}
