/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "sys/configuration.h"

#include "tune.h"

// TODO: autotune was too messy and iffy for now, going to come back to it later :)

void tune_update() {
}

bool tune_is_tuned() {
    return (bool)calibration.pid[PID_TUNED];
}
