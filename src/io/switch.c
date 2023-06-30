#include <stdint.h>

#include "../modes/modes.h"

uint8_t pos;

void switchPos(uint8_t currentPos) {
    // This way, the mode will only be changed when the user moves the switch;
    // the system's mode changes can persist.
    if (currentPos != pos) {
        mode(currentPos);
        pos = currentPos;
    }
}
