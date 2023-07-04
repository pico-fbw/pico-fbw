/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdint.h>

#include "../modes/modes.h"

#include "switch.h"

uint8_t pos;

void updateSwitch(uint8_t currentPos) {
    // This way, the mode will only be changed when the user moves the switch;
    // the system's mode changes can persist and won't instantly be overrided.
    if (currentPos != pos) {
        pos = currentPos;
        toMode(pos);
    }
}
