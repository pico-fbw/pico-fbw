/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include "platform/time.h"

#include "io/aahrs.h"
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

#define ROLL_OVERSHOOT_THRESHOLD 8.0 // The threshold at which to apply reverse input to dampen roll overshoot, deg
// The aircraft's current roll rate will be divided by this value and subsequently multiplied by
// the reverse output of the lateral guidance controller
#define ROLL_OVERSHOOT_DAMPEN 70.0 // The factor at which to dampen roll overshoot, deg/s

typedef enum GuidanceSource {
    SOURCE_FLIGHTPLAN,
    SOURCE_EXTERNAL,
} GuidanceSource;

static bool autoComplete = false;

// Current Waypoint we're tracking to
static Waypoint currentWpt;
static u32 currentWptIndex;

static PIDController latGuid;  // lateral guidance
static PIDController vertGuid; // vertical guidance

// Allows auto mode to be externally controlled (by API setting a custom Waypoint and callback)
static GuidanceSource guidanceSource = SOURCE_FLIGHTPLAN;
static Waypoint externWpt;
static void (*captureCallback)(void) = NULL;

// Callback for when the bay needs to be closed after a user-specified delay (within the flightplan)
static i32 callback_drop(void *data) {
    auto_set_bay_position(POS_CLOSED);
    return 0; // Don't repeat
    (void)data;
}

/**
 * Load the given Waypoint and begin tracking to it.
 * @param wpt the Waypoint to load
 */
static void load_waypoint(Waypoint *wpt) {
    currentWpt = *wpt;
    // Factor in the altitude offset if calculated earlier
    if (gps.altOffsetCalibrated)
        currentWpt.alt = wpt->alt + gps.altOffset;
    // Set the (possibly new) target speed
    throttle.target = wpt->speed;
    // Initiate a drop if applicable
    if (wpt->drop > 0) {
        auto_set_bay_position(POS_OPEN);
        // Schedule a callback, since the bay needs to close after some time
        callback_in_ms(wpt->drop * 1000, callback_drop, NULL);
    }
}

/**
 * Load the next Waypoint in the flightplan.
 */
static inline void load_next_waypoint() {
    load_waypoint(&(flightplan_get()->waypoints[currentWptIndex]));
}

bool auto_init() {
    // Import flightplan data
    if (!flightplan_was_parsed()) {
        log_message(TYPE_ERROR, "No flightplan parsed!", 2000, 0, false);
        return false;
    }
    guidanceSource = SOURCE_FLIGHTPLAN;
    currentWptIndex = 0;
    flight_init();
    throttle.init();
    // Check if SPEED mode is supported, which we need for autopilot
    if (throttle.supportedMode < THRMODE_SPEED) {
        log_message(TYPE_WARNING, "SPEED mode required!", 2000, 0, false);
        return false;
    }
    throttle.mode = THRMODE_SPEED;
    // Initialize (clear) PIDs
    latGuid = (PIDController){
        .kp = LATGD_KP,
        .ki = LATGD_KI,
        .kd = LATGD_KD,
        .tau = LATGD_TAU,
        .limMin = -LATGD_LIM,
        .limMax = LATGD_LIM,
    };
    vertGuid = (PIDController){
        .kp = VERTGD_KP,
        .ki = VERTGD_KI,
        .kd = VERTGD_KD,
        .tau = VERTGD_TAU,
        .limMin = VERTGD_LIM_MIN,
        .limMax = VERTGD_LIM_MAX,
    };
    pid_init(&latGuid);
    pid_init(&vertGuid);
    // Load the first Waypoint from the flightplan (subsequent waypoints will be loaded on waypoint interception)
    load_next_waypoint();
    return true;
}

void auto_update() {
    // Don't allow re-entering auto mode after the user has exited hold mode and auto is complete
    if (autoComplete) {
        aircraft.change_to(MODE_NORMAL);
        return;
    }

    // Calculate the bearing and distance to either the current Waypoint in the flightplan or an externally set Waypoint
    f64 bearing, distance;
    Waypoint target;
    switch (guidanceSource) {
        case SOURCE_FLIGHTPLAN:
            target = currentWpt;
            break;
        case SOURCE_EXTERNAL:
            target = externWpt;
            break;
    }
    bearing = calculate_bearing(gps.lat, gps.lng, target.lat, target.lng);
    distance = calculate_distance(gps.lat, gps.lng, target.lat, target.lng);

    // Calculate difference between track and bearing, normalized between -180 and 180
    // Use GPS track instead of IMU heading because heading isn't always going to be navigational (more likely magnetic)
    f64 diff = gps.track - bearing;
    if (diff > 180.0)
        diff -= 360.0;
    else if (diff < -180.0)
        diff += 360.0;

    // Predictive roll control adjustment to avoid overshooting
    if (fabs(diff) < ROLL_OVERSHOOT_THRESHOLD)
        // Apply reverse input to dampen overshoot
        latGuid.out = -latGuid.out * (aahrs.rollRate / ROLL_OVERSHOOT_DAMPEN);

    // Nested PIDs to command bank/pitch angles
    pid_update(&latGuid, 0.0, diff);
    pid_update(&vertGuid, target.alt, gps.alt);
    flight_update(latGuid.out, vertGuid.out, 0, false);
    throttle.update();

    // Calculate the radius at which to consider the Waypoint intercepted
    // This must be calculated every loop as we need to turn sooner if we're going faster to stay on course
    f64 radius = INTERCEPT_BASE_RADIUS + (throttle.target - INTERCEPT_BASE_SPEED) * 5;
    radius = (radius < MIN_RADIUS) ? MIN_RADIUS : radius;
    // If we've intercepted the waypoint,
    if (distance < radius) {
        switch (guidanceSource) {
            case SOURCE_FLIGHTPLAN:
                // then advance to the next one
                currentWptIndex++;
                // Check if the flightplan is over
                if (currentWptIndex >= flightplan_get()->waypoint_count) {
                    // Auto mode ends here, we enter a holding pattern
                    autoComplete = true;
                    aircraft.change_to(MODE_HOLD);
                } else
                    // More waypoints to go, load the next one
                    load_next_waypoint();
                break;
            case SOURCE_EXTERNAL:
                // then execute the callback function and enter a holding pattern
                if (captureCallback)
                    (captureCallback)();
                guidanceSource = SOURCE_FLIGHTPLAN;
                aircraft.change_to(MODE_HOLD);
                break;
        }
    }
}

void auto_set(Waypoint wpt, void (*callback)(void)) {
    guidanceSource = SOURCE_EXTERNAL;
    externWpt = wpt;
    captureCallback = callback;
    load_waypoint(&externWpt);
}

void auto_set_bay_position(BayPosition pos) {
    switch (pos) {
        case POS_OPEN:
            servo_set(config.pins[PINS_SERVO_BAY], config.control[CONTROL_DROP_DETENT_OPEN]);
            break;
        case POS_CLOSED:
        default:
            servo_set(config.pins[PINS_SERVO_BAY], config.control[CONTROL_DROP_DETENT_CLOSED]);
            break;
    }
}
