#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "../io/led.h"
#include "../io/wifly/wifly.h"

#include "modes.h"
#include "tune.h"

#include "auto.h"

// Quick note--quite a bit of auto mode works very similarly if not the same as normal mode, so I only documented the new additions
// If something isn't documented, chances are it's in normal.c!

Waypoint *fplan = NULL;

bool autoInitialized = false;


static void auto_computePID() {
    // tbd once init function is complete
}

bool autoInit() {
    // Import the flightplan data from Wi-Fly and check if it's valid
    fplan = wifly_getFplan();
    if (fplan == NULL) {
        return false;
    }
    // TODO: set up PIDs and find optimal tunings (shouldn't need to autotune for this, it will be nested PID)
}

void mode_autoDeinit() {
    autoInitialized = false;
    multicore_reset_core1();
}

void mode_auto() {
    if (!autoInitialized) {
        if (autoInit()) {
            autoInitialized = true;
        } else {
            return;
        }
    }
    // Auto mode is not fully implemented yet! Switch back to normal mode.
    mode(NORMAL);
    // TODO: auto mode runtime (I love how I say that like it's simple...yikes)
}
