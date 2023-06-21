/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "../io/flash.h"
#include "../io/imu.h"
#include "../io/gps.h"
#include "../io/led.h"
#include "../io/wifly/wifly.h"

#include "../lib/pid.h"
#include "../lib/nav.h"

#include "../config.h"

#include "modes.h"
#include "normal.h"
#include "tune.h"

#include "auto.h"

/**
 * Quick note--quite a bit of auto mode works very similarly if not the same as normal mode, so I only documented the new additions
 * If something isn't documented, chances are it's in normal.c!
 * As much as I hate repeating myself it just makes more sense to have seperate PIDs for each mode
*/

bool autoInitialized = false;
bool autoFirstTimeInit = true;

Waypoint *fplan = NULL;
uint currentWptIdx = 0;
Waypoint currentWpt;

inertialAngles aircraft;
gpsCoords loc;
double currentLat;
double currentLng;

PIDController rollGuid;
PIDController pitchGuid;
PIDController horzGuid; // TODO: combine lat and lng pids into one horizontal pid that directly controls the roll pid?
PIDController latGuid;
float latSet;
PIDController lngGuid;
float lngSet;
PIDController vertGuid;
float altSet;

#ifdef WIFLY_ENABLED

static inline void auto_computePID() {
    while (true) {
        // TODO: computersing proportional integral derivite
        // ^ was I high when I wrote this or something???
    }
}

bool autoInit() {
    mode_normalDeinit();
    // Import the flightplan data from Wi-Fly and check if it's valid
    fplan = wifly_getFplan();
    if (fplan == NULL || wifly_getWaypointCount == 0) {
        return false;
    }
    #ifdef PID_AUTOTUNE
        if (mode_tuneCheckCalibration()) {
            rollGuid = (PIDController){flash_read(1, 1), flash_read(1, 2), flash_read(1, 3), roll_tau, -AIL_LIMIT, AIL_LIMIT, roll_integMin, roll_integMax, roll_kT};
            pitchGuid = (PIDController){flash_read(2, 1), flash_read(2, 2), flash_read(2, 3), pitch_tau, -ELEV_LIMIT, ELEV_LIMIT, pitch_integMin, pitch_integMax, pitch_kT};
        } else {
            if (autoFirstTimeInit) {
                led_blink(2000);
                autoFirstTimeInit = false;
            }
            mode(DIRECT);
            return false;
        }
    #else
        rollGuid = (PIDController){roll_kP, roll_kI, roll_kD, roll_tau, -AIL_LIMIT, AIL_LIMIT, roll_integMin, roll_integMax, roll_kT};
        pitchGuid = (PIDController){pitch_kP, pitch_kI, pitch_kD, pitch_tau, -ELEV_LIMIT, ELEV_LIMIT, pitch_integMin, pitch_integMax, pitch_kT};
    #endif
    // Create PID controllers for lateral and vertical guidance
    horzGuid = (PIDController){horzGuid_kP, horzGuid_kI, horzGuid_kD, horzGuid_tau, -horzGuid_lim, horzGuid_lim, horzGuid_integMin, horzGuid_integMax, horzGuid_kT};
    vertGuid = (PIDController){vertGuid_kP, vertGuid_kI, vertGuid_kD, vertGuid_tau, vertGuid_loLim, vertGuid_upLim, vertGuid_integMin, vertGuid_integMax, vertGuid_kT};
    pid_init(&rollGuid);
    pid_init(&pitchGuid);
    pid_init(&latGuid);
    pid_init(&vertGuid);
    multicore_launch_core1(auto_computePID);
    return true;
}

void mode_auto() {
    if (!autoInitialized) {
        if (autoInit()) {
            autoInitialized = true;
        } else {
            return;
        }
    }
    // TODO: auto mode runtime (I love how I say that like it's simple...yikes)
    /**
     * Plan:
     * [x] get current aircraft heading
     * [x] calculate heading to next waypoint (that's the error)
     * [ ] input error to PID controller which will calculate a deg value for ail
     * [ ] input deg into ail pid
    */
    aircraft = imu_getAngles();
    loc = gps_getCoords();
    if (aircraft.roll > 72 || aircraft.roll < -72 || aircraft.pitch > 35 || aircraft.pitch < -20) {
        setIMUSafe(false);
    }
    if (currentWptIdx > wifly_getWaypointCount()) {
        // auto mode should end here? all waypoints have been achieved
    }
    double bearing = calculateBearing(loc.lat, loc.lng, fplan[currentWptIdx].lat, fplan[currentWptIdx].lng);
    // ...

}

#endif

void mode_autoDeinit() {
    autoInitialized = false;
    multicore_reset_core1();
}
