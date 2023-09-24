/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include "pico/types.h"

#include "../io/flash.h"
#include "../io/imu.h"
#include "../io/gps.h"

#include "../lib/pid.h"
#include "../lib/nav.h"

#include "modes.h"
#include "normal.h"
#include "tune.h"
#include "flight.h"

#include "../sys/config.h"

#include "../wifly/wifly.h"

#include "auto.h"

// TODO: Aadd documentation for auto mode on the wiki!
// "materials" and "how to use system" need updating and also a completely new page for wifly and how to use it
// also make sure to mention it in the readme

static bool autoComplete = false;

static Waypoint *fplan = NULL;
static uint currentWaypoint = 0;
static Waypoint currentWpt;

static double distance;
static double bearing;
static int alt;

static PIDController latGuid;
static PIDController vertGuid;

bool mode_autoInit() {
    // Import the flightplan data from Wi-Fly and check if it's valid
    fplan = wifly_getFplan();
    if (fplan == NULL || wifly_getWaypointCount() == 0) {
        return false;
    }
    // Initialize (clear) PIDs
    flight_init();
    latGuid = (PIDController){latGuid_kP, latGuid_kI, latGuid_kD, latGuid_tau, -latGuid_lim, latGuid_lim, latGuid_integMin, latGuid_integMax, latGuid_kT};
    vertGuid = (PIDController){vertGuid_kP, vertGuid_kI, vertGuid_kD, vertGuid_tau, vertGuid_loLim, vertGuid_upLim, vertGuid_integMin, vertGuid_integMax, vertGuid_kT};
    pid_init(&latGuid);
    pid_init(&vertGuid);
    // Load the first altitude from the flightplan (subsequent altitudes will be loaded on waypoint interception)
    // if it is -5 (default) just discard it (by setting it to our current alt; no change)
    if (gps_isAltOffsetCalibrated()) {
        if (fplan[currentWaypoint].alt < -5) {
            alt = gps.alt;
        } else {
            // Factor in the altitude offset calculated earlier, if applicable
            alt = fplan[currentWaypoint].alt + gps_getAltOffset();
        }
    } else {
        if (fplan[currentWaypoint].alt < -5) {
            alt = gps.alt;
        } else {
            alt = fplan[currentWaypoint].alt;
        }
    }
    return true;
}

void mode_auto() {
    // Don't allow re-entering auto mode after the user has exited hold mode and auto is complete
    if (autoComplete) {
        toMode(MODE_NORMAL);
        return;
    }

    // Calculate the bearing and distanceto the current waypoint
    bearing = calculateBearing(gps.lat, gps.lng, fplan[currentWaypoint].lat, fplan[currentWaypoint].lng);
    distance = calculateDistance(gps.lat, gps.lng, fplan[currentWaypoint].lat, fplan[currentWaypoint].lng);

    // Nested PIDs; latGuid and vertGuid use imu & gps data to command bank/pitch angles which the flight PIDs then use to actuate servos
    pid_update(&latGuid, bearing, gps.trk_true); // Don't use aircraft heading because that's not always going to be navigational (either true or magnetic)
    pid_update(&vertGuid, alt, gps.alt);
    flight_update(latGuid.out, vertGuid.out, 0, false);

    // If we've "intercepted" the waypoint then advance to the next one
    if (distance <= INTERCEPT_RADIUS) {
        currentWaypoint++;
        if (currentWaypoint > wifly_getWaypointCount()) {
            // Auto mode ends here, we enter a holding pattern
            autoComplete = true;
            toMode(MODE_HOLD);
            return;
        } else {
            // Load the next altitude
            if (gps_isAltOffsetCalibrated()) {
                if (fplan[currentWaypoint].alt < -5) {
                    alt = gps.alt;
                } else {
                    // Factor in the altitude offset calculated earlier, if applicable
                    alt = fplan[currentWaypoint].alt + gps_getAltOffset();
                }
            } else {
                if (fplan[currentWaypoint].alt < -5) {
                    alt = gps.alt;
                } else {
                    alt = fplan[currentWaypoint].alt;
                }
            }
        }
    }
}
