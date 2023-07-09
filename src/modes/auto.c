/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include "pico/multicore.h"
#include "../lib/pid.h"
#include "../lib/nav.h"

#include "../io/flash.h"
#include "../io/imu.h"
#include "../io/gps.h"
#include "../io/led.h"
#include "../io/wifly/wifly.h"

#include "../config.h"

#include "modes.h"
#include "normal.h"
#include "tune.h"
#include "flight.h"

#include "auto.h"

#ifdef WIFLY_ENABLED

static bool autoComplete = false;

static Waypoint *fplan = NULL;
static uint currentWptIdx = 0;
static Waypoint currentWpt;

// Current waypoint information
static double distance;
static double bearing;
static int_fast16_t alt;

static PIDController latGuid;
static PIDController vertGuid;

static inline void auto_compute() {
    while (true) {
        // Nested PIDs; latGuid and vertGuid use imu & gps data to command bank/pitch angles which the flight PIDs then use to actuate servos
        pid_update(&latGuid, bearing, aircraft.heading);
        pid_update(&vertGuid, alt, gps.alt);
        flight_update_core1(latGuid.out, vertGuid.out, 0, false);
    }
}

#endif // WIFLY_ENABLED

bool mode_autoInit() {
    #ifdef WIFLY_ENABLED
        // Import the flightplan data from Wi-Fly and check if it's valid
        fplan = wifly_getFplan();
        if (fplan == NULL || wifly_getWaypointCount() == 0) {
            return false;
        }
        // Initialize (clear) PIDs and launch them on core 1
        flight_init();
        latGuid = (PIDController){latGuid_kP, latGuid_kI, latGuid_kD, latGuid_tau, -latGuid_lim, latGuid_lim, latGuid_integMin, latGuid_integMax, latGuid_kT};
        vertGuid = (PIDController){vertGuid_kP, vertGuid_kI, vertGuid_kD, vertGuid_tau, vertGuid_loLim, vertGuid_upLim, vertGuid_integMin, vertGuid_integMax, vertGuid_kT};
        pid_init(&latGuid);
        pid_init(&vertGuid);
        multicore_launch_core1(auto_compute);
        return true;
    #else
        return false;
    #endif // WIFLY_ENABLED
}

#ifdef WIFLY_ENABLED

void mode_auto() {
    // Don't allow re-entering auto mode after the user has exited hold mode and auto is complete
    if (autoComplete) {
        toMode(NORMAL);
        return;
    }
    
    // Refresh flight and gps data
    flight_update_core0();

    // Calculate the distance to the current waypoint
    distance = calculateDistance(gps.lat, gps.lng, fplan[currentWptIdx].lat, fplan[currentWptIdx].lng);
    // If we've "intercepted" the waypoint then advance to the next one
    if (distance <= INTERCEPT_RADIUS) {
        currentWptIdx++;
        if (currentWptIdx > wifly_getWaypointCount()) {
            // Auto mode ends here, we enter a holding pattern
            autoComplete = true;
            toMode(HOLD);
            return;
        } else {
            // Load the altitude--if it is -5 (default) just discard it (by setting it to our current alt; no change)
            if (fplan[currentWptIdx].alt < -5) {
                alt = gps.alt;
            } else {
                alt = fplan[currentWptIdx].alt;
            }
        }
    }
    // Calculate the up-to-date bearing to the current waypoint
    bearing = calculateBearing(gps.lat, gps.lng, fplan[currentWptIdx].lat, fplan[currentWptIdx].lng);
}

// TODO: Aadd documentation for auto mode on the wiki!
// "materials" and "how to use system" need updating and also a completely new page for wifly and how to use it
// also make sure to mention it in the readme

#endif // WIFLY_ENABLED

void mode_autoDeinit() {
    multicore_reset_core1(); // Reset core 1 for use elsewhere
}
