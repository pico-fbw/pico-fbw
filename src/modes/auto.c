/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

// TODO: refactor using new autopilot logic

#include "platform/helpers.h"
#include "platform/time.h"

#include "ctrl/aircraft.h"
#include "ctrl/flight.h"
#include "ctrl/throttle.h"
#include "io/gps.h"
#include "io/imu.h"
#include "io/servo.h"
#include "lib/nav.h"
#include "lib/pid.h"
#include "modes/normal.h"
#include "modes/tune.h"
#include "sys/configuration.h"
#include "sys/flightplan.h"
#include "sys/log.h"

#include "auto.h"

#define INTERCEPT_BASE_RADIUS 25 // The baseline radius at which to consider a Waypoint incercepted, in meters
#define INTERCEPT_BASE_SPEED 50  // INTERCEPT_BASE_RADIUS will apply at this speed, kts
#define MIN_RADIUS 5             // The minimum radius that is possible (after being calculated), in meters

typedef enum GuidanceSource {
    SOURCE_FLIGHTPLAN,
    SOURCE_EXTERNAL,
} GuidanceSource;

static bool autoComplete = false;

// Current Waypoint we're tracking to
static Waypoint currentWpt;
static u32 currentWptIndex;

static PIDController latGuid;  // Lateral guidance
static PIDController vertGuid; // Vertical guidance
static f32 rollOut, pitchOut; // Smoothed outputs from guidance PIDs

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
    if (gps.altOffsetCalibrated) {
        currentWpt.alt = wpt->alt + gps.altOffset;
    }
    // Set the (possibly new) target speed
    throttle_set_target(wpt->speed);
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
    load_waypoint(&(flightplan_get_active()->waypoints[currentWptIndex]));
}

bool auto_init() {
    // Import flightplan data
    if (!flightplan_get_active()) {
        log_message(TYPE_ERROR, "No active flightplan!", 2000, 0, false);
        return false;
    }
    guidanceSource = SOURCE_FLIGHTPLAN;
    currentWptIndex = 0;
    flight_init();
    throttle_init();
    // Check if SPEED mode is supported, which we need for autopilot
    if (throttle_get_supported_mode() < THRMODE_SPEED) {
        log_message(TYPE_WARNING, "SPEED mode required!", 2000, 0, false);
        return false;
    }
    throttle_set_mode(THRMODE_SPEED);
    // Initialize (clear) PIDs
    latGuid = (PIDController){
        .kp = LATGD_KP,
        .ki = LATGD_KI,
        .kd = LATGD_KD,
        .tau = calibration.pid[PID_TAU],
        .limMin = -LATGD_LIM,
        .limMax = LATGD_LIM,
    };
    vertGuid = (PIDController){
        .kp = VERTGD_KP,
        .ki = VERTGD_KI,
        .kd = VERTGD_KD,
        .tau = calibration.pid[PID_TAU],
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
        aircraft_change_mode(MODE_NORMAL);
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
    f32 diff = control_get_heading_diff(bearing, gps.track);

    // Nested PIDs to command bank/pitch angles
    pid_update(&latGuid, 0.0, diff);
    pid_update(&vertGuid, target.alt, gps.alt);
    // lerp() the PID outputs in order to avoid jerkiness on the controls
    // Invert lateral output because positive diff should mean positive bank (and vice versa)
    rollOut = lerp(rollOut, -latGuid.out, GUIDANCE_SMOOTHING);
    pitchOut = lerp(pitchOut, vertGuid.out, GUIDANCE_SMOOTHING);
    
    flight_update(rollOut, pitchOut, 0, false);
    throttle_update();

    // Calculate the radius at which to consider the Waypoint intercepted
    // This must be calculated every loop as we need to turn sooner if we're going faster to stay on course
    f64 radius = INTERCEPT_BASE_RADIUS + (throttle_get_target() - INTERCEPT_BASE_SPEED) * 5;
    radius = (radius < MIN_RADIUS) ? MIN_RADIUS : radius;
    // If we've intercepted the waypoint,
    if (distance < radius) {
        switch (guidanceSource) {
            case SOURCE_FLIGHTPLAN:
                // then advance to the next one
                currentWptIndex++;
                // Check if the flightplan is over
                if (currentWptIndex >= flightplan_get_active()->waypoint_count) {
                    // Auto mode ends here, we enter a holding pattern
                    autoComplete = true;
                    aircraft_change_mode(MODE_HOLD);
                } else {
                    // More waypoints to go, load the next one
                    load_next_waypoint();
                }
                break;
            case SOURCE_EXTERNAL:
                // then execute the callback function and enter a holding pattern
                if (captureCallback) {
                    (captureCallback)();
                }
                guidanceSource = SOURCE_FLIGHTPLAN;
                aircraft_change_mode(MODE_HOLD);
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
