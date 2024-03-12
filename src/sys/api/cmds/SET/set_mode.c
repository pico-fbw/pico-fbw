/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdlib.h>

#include "modes/aircraft.h"

#include "set_mode.h"

i32 api_set_mode(const char *args) {
    Mode mode = atoi(args);
    // Ensure mode is valid before setting it
    if (mode >= MODE_MIN && mode < MODE_MAX) {
        aircraft.changeTo(mode);
        return 200;
    } else
        return 400;
}
