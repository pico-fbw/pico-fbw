/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>

#include "../io/flash.h"

#include "tune.h"

// TODO: autotune was too messy and iffy for now, going to come back to it later :)

void tune_update() {
    
}

bool tune_isCalibrated() {
    return (bool)flash.pid[PID_FLAG];
}
