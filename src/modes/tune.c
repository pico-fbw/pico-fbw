/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>

#include "../io/flash.h"

#include "tune.h"

// TODO: autotune was too messy and iffy for now, going to come back to it later :)

void mode_tune() {
    
}

bool mode_tuneisCalibrated() {
    if ((flash_readFloat(FLOAT_SECTOR_PID, 0) == FLAG_PID)) {
        // TODO: also ensure the values make sense?
        return true;
    } else {
        return false;
    }
}
