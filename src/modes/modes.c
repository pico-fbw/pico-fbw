#include "pico/stdlib.h"

#include "modes.h"

/* Define local variables--these should not be accessable to other files. */

uint cmode = 1;
bool imuDataSafe = false;

/* End variable definitions. */

void setMode(uint mode) {
    cmode = mode;
}

void setIMUSafe(bool state) {
    imuDataSafe = state;
}