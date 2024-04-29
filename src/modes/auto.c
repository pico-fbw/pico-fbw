/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/time.h"

#include "io/gps.h"
#include "io/servo.h"

#include "lib/nav.h"
#include "lib/pid.h"

#include "modes/aircraft.h"
#include "modes/flight.h"
#include "modes/normal.h"
#include "modes/tune.h"

#include "sys/configuration.h"
#include "sys/flightplan.h"
#include "sys/log.h"
#include "sys/throttle.h"

#include "auto.h"

#define INTERCEPT_BASE_RADIUS 25 // The baseline radius at which to consider a Waypoint incercepted, in meters
#define INTERCEPT_BASE_SPEED 50  // INTERCEPT_BASE_RADIUS will apply at this speed, kts
#define MIN_RADIUS 5             // The minimum radius that is possible (after being calculated), in meters

typedef enum GuidanceSource {
    FPLAN,
    EXTERNAL,
} GuidanceSource;

static bool autoComplete = false;

// Details of the current Waypoint we're tracking to
static u32 currentWaypoint = 0;
static f64 distance;
static f64 bearing;
static i32 alt;
static PIDController latGuid;
static PIDController vertGuid;

// Allows auto mode to be externally controlled (by API setting a custom Waypoint and callback)
static GuidanceSource guidanceSource = FPLAN;
static Waypoint externWpt;
static void (*captureCallback)(void) = NULL;

// Callback for when the bay needs to be closed after a user-specified delay (within the flightplan)
i32 dropCallback() {
    auto_set_bay_position(CLOSED);
    return 0; // Don't repeat
}

static inline void loadWaypoint(Waypoint *wpt) {
    // Load the next altitude, if it is -5 (the default) just discard it (by setting it to our current alt; no change)
    if (gps.altOffsetCalibrated) {
        // Factor in the altitude offset calculated earlier
        alt = (wpt->alt <= -5) ? gps.alt : (wpt->alt + gps.altOffset);
    } else
        alt = (wpt->alt <= -5) ? gps.alt : wpt->alt;
    // Set the (possibly new) target speed
    throttle.target = (wpt->speed == -5) ? gps.speed : wpt->speed;
    // Initiate a drop if applicable
    if (wpt->drop != 0) {
        auto_set_bay_position(OPEN);
        if (wpt->drop > 0)
            // Schedule a callback if the bay needs to close after some time
            callback_in_ms(wpt->drop * 1000, dropCallback);
    }
}

bool auto_init() {
    // Import flightplan data
    if (!flightplan_was_parsed()) {
        log_message(ERROR, "No flightplan parsed!", 2000, 0, false);
        return false;
    }
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
// The PIDController struct contains some internal variables that we don't initialize, so we suppress the warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    latGuid = (PIDController){latGuid_kP,   latGuid_kI,  latGuid_kD,        latGuid_tau,
                              -latGuid_lim, latGuid_lim, -latGuid_integLim, latGuid_integLim};
    vertGuid = (PIDController){vertGuid_kP,    vertGuid_kI,    vertGuid_kD,        vertGuid_tau,
                               vertGuid_loLim, vertGuid_hiLim, -vertGuid_integLim, vertGuid_integLim};
#pragma GCC diagnostic pop
    pid_init(&latGuid);
    pid_init(&vertGuid);
    // Load the first Waypoint from the flightplan (subsequent waypoints will be loaded on waypoint interception)
    loadWaypoint(&flightplan_get()->waypoints[currentWaypoint]);
    return true;
}

void auto_update() {
    // Don't allow re-entering auto mode after the user has exited hold mode and auto is complete
    if (autoComplete) {
        aircraft.change_to(MODE_NORMAL);
        return;
    }

    // Calculate the bearing and distance...
    switch (guidanceSource) {
        case FPLAN:
            // ...to the current Waypoint in the flightplan
            bearing = calculate_bearing(gps.lat, gps.lng, flightplan_get()->waypoints[currentWaypoint].lat,
                                        flightplan_get()->waypoints[currentWaypoint].lng);
            distance = calculate_distance(gps.lat, gps.lng, flightplan_get()->waypoints[currentWaypoint].lat,
                                          flightplan_get()->waypoints[currentWaypoint].lng);
            break;
        case EXTERNAL:
            // ...to the current Waypoint (temporarily set)
            bearing = calculate_bearing(gps.lat, gps.lng, externWpt.lat, externWpt.lng);
            distance = calculate_distance(gps.lat, gps.lng, externWpt.lat, externWpt.lng);
            break;
    }

    // Nested PIDs; latGuid and vertGuid use gps data to command bank/pitch angles which the flight PIDs then use to actuate
    // servos
    pid_update(&latGuid, bearing,
               gps.track); // Don't use IMU heading because that's not always going to be navigational (more likely magnetic)
    pid_update(&vertGuid, alt, gps.alt);
    flight_update(latGuid.out, vertGuid.out, 0, false);
    throttle.update();

    // Calculate the radius at which to consider the Waypoint intercepted
    // This must be calculated every loop as we need to turn sooner if we're going faster to stay on course
    f64 radius = INTERCEPT_BASE_RADIUS + (throttle.target - INTERCEPT_BASE_SPEED) * 5;
    radius = (radius < MIN_RADIUS) ? MIN_RADIUS : radius;
    // If we've intercepted the waypoint,
    if (distance < radius) {
        switch (guidanceSource) {
            case FPLAN:
                // then advance to the next one
                currentWaypoint++;
                // Check if the flightplan is over
                if (currentWaypoint > flightplan_get()->waypoint_count) {
                    // Auto mode ends here, we enter a holding pattern
                    autoComplete = true;
                    aircraft.change_to(MODE_HOLD);
                } else
                    // More waypoints to go, load the next one
                    loadWaypoint(&flightplan_get()->waypoints[currentWaypoint]);
                break;
            case EXTERNAL:
                // then execute the callback function and enter a holding pattern
                if (captureCallback)
                    (captureCallback)();
                guidanceSource = FPLAN;
                aircraft.change_to(MODE_HOLD);
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

void auto_set_bay_position(BayPosition pos) {
    switch (pos) {
        case OPEN:
            servo_set(config.pins[PINS_SERVO_BAY], config.control[CONTROL_DROP_DETENT_OPEN]);
            break;
        case CLOSED:
        default:
            servo_set(config.pins[PINS_SERVO_BAY], config.control[CONTROL_DROP_DETENT_CLOSED]);
            break;
    }
}
