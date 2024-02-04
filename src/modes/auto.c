/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include "pico/time.h"
#include "pico/types.h"

#include "io/flash.h"
#include "io/gps.h"
#include "io/servo.h"

#include "lib/pid.h"
#include "lib/nav.h"

#include "sys/log.h"
#include "sys/throttle.h"

#include "modes/aircraft.h"
#include "modes/normal.h"
#include "modes/tune.h"
#include "modes/flight.h"

#include "modes/auto.h"

typedef enum GuidanceSource {
    FPLAN,
    EXTERNAL
} GuidanceSource;

static bool autoComplete = false;

static GuidanceSource guidanceSource = FPLAN;

static Waypoint *fplan = NULL;
static uint currentWaypoint = 0;

static Waypoint externWpt;
static void (*captureCallback)(void);

static double distance;
static double bearing;
static int alt;

static PIDController latGuid;
static PIDController vertGuid;

// Callback for when the bay needs to be closed after a user-specified delay (within the flightplan)
static inline int64_t dropCallback(alarm_id_t id, void *data) {
    auto_setBayPosition(CLOSED);
    return 0;
}

static inline void loadWaypoint(Waypoint *wpt) {
    // Load the next altitude, if it is -5 (default) just discard it (by setting it to our current alt; no change)
    if (gps.altOffset_calibrated) {
        // Factor in the altitude offset calculated earlier
        alt = (wpt->alt <= -5) ? gps.alt : (wpt->alt + gps.altOffset);
    } else {
        alt = (wpt->alt <= -5) ? gps.alt : wpt->alt;
    }
    // Set the (possibly new) target speed
    throttle.target = (wpt->speed == -5) ? gps.speed : wpt->speed;
    // Initiate a drop if applicable
    if (wpt->drop != 0) {
        auto_setBayPosition(OPEN);
        if (wpt->drop > 0) {
            // Schedule a callback if the bay needs to close after some time
            add_alarm_in_ms(wpt->drop * 1000, dropCallback, NULL, true);
        }
    }
}

bool auto_init() {
    // Import the flightplan data from Wi-Fly
    if (!wifly_fplanExists())
        return false;
    fplan = wifly_getFplan();
    guidanceSource = FPLAN;
    currentWaypoint = 0;
    flight_init();
    throttle.init();
    // Check if SPEED mode is supported, which we need for autopilot
    if (throttle.supportedMode < THRMODE_SPEED) {
        log_message(WARNING, "SPEED mode required!", 2000, 0, false);
        return false;
    }
    throttle.mode = THRMODE_SPEED;
    // Initialize (clear) PIDs
    latGuid = (PIDController){latGuid_kP, latGuid_kI, latGuid_kD, latGuid_tau, -latGuid_lim, latGuid_lim, latGuid_integMin, latGuid_integMax, latGuid_kT};
    vertGuid = (PIDController){vertGuid_kP, vertGuid_kI, vertGuid_kD, vertGuid_tau, vertGuid_loLim, vertGuid_upLim, vertGuid_integMin, vertGuid_integMax, vertGuid_kT};
    pid_init(&latGuid);
    pid_init(&vertGuid);
    // Load the first waypoint from the flightplan (subsequent waypoints will be loaded on waypoint interception)
    loadWaypoint(&fplan[currentWaypoint]);
    return true;
}

void auto_update() {
    // Don't allow re-entering auto mode after the user has exited hold mode and auto is complete
    if (autoComplete) {
        aircraft.changeTo(MODE_NORMAL);
        return;
    }

    // Calculate the bearing and distance...
    switch (guidanceSource) {
        case FPLAN:
            // ...to the current waypoint in the flightplan
            bearing = calculateBearing(gps.lat, gps.lng, fplan[currentWaypoint].lat, fplan[currentWaypoint].lng);
            distance = calculateDistance(gps.lat, gps.lng, fplan[currentWaypoint].lat, fplan[currentWaypoint].lng);
            break;
        case EXTERNAL:
            // ...to the current waypoint (temporarily set)
            bearing = calculateBearing(gps.lat, gps.lng, externWpt.lat, externWpt.lng);
            distance = calculateDistance(gps.lat, gps.lng, externWpt.lat, externWpt.lng);
            break;
    }

    // Nested PIDs; latGuid and vertGuid use gps data to command bank/pitch angles which the flight PIDs then use to actuate servos
    pid_update(&latGuid, bearing, gps.track); // Don't use IMU heading because that's not always going to be navigational (more likely magnetic)
    pid_update(&vertGuid, alt, gps.alt);
    flight_update(latGuid.out, vertGuid.out, 0, false);
    throttle.update();

    // If we've "intercepted" the waypoint,
    if (distance <= INTERCEPT_RADIUS) {
        switch (guidanceSource) {
            case FPLAN:
                // then advance to the next one
                currentWaypoint++;
                // Check if the flightplan is over
                if (currentWaypoint > wifly_getWaypointCount()) {
                    // Auto mode ends here, we enter a holding pattern
                    autoComplete = true;
                    aircraft.changeTo(MODE_HOLD);
                } else {
                    // Load the next altitude
                    loadWaypoint(&fplan[currentWaypoint]);
                }
                break;
            case EXTERNAL:
                // then execute the callback function and enter a holding pattern
                (captureCallback)();
                guidanceSource = FPLAN;
                aircraft.changeTo(MODE_HOLD);
                break;
        }
    }
}

void auto_set(Waypoint wpt, void (*callback)(void)) {
    guidanceSource = EXTERNAL;
    externWpt = wpt;
    captureCallback = callback;
    loadWaypoint(&externWpt);
}

void auto_setBayPosition(BayPosition pos) {
    switch (pos) {
        case OPEN:
            servo_set(flash.pins[PINS_SERVO_BAY], flash.control[CONTROL_DROP_DETENT_OPEN]);
            break;
        case CLOSED:
        default:
            servo_set(flash.pins[PINS_SERVO_BAY], flash.control[CONTROL_DROP_DETENT_CLOSED]);
            break;
    }
}
