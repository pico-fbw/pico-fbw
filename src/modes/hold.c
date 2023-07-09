/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "pico/multicore.h"
#include "pico/time.h"
#include "../lib/pid.h"

#include "flight.h"

#include "../config.h"

#include "hold.h"

#ifdef WIFLY_ENABLED

static uint8_t turnStatus = HOLD_TURN_UNSCHEDULED;

static float oldHeading;
static float targetHeading;
static int_fast16_t targetAlt;

static double rollSet;

static PIDController vertGuid;

static inline void hold_compute() {
    while (true) {
        pid_update(&vertGuid, targetAlt, gps.alt);
        flight_update_core1(rollSet, vertGuid.out, 0, false);
    }
}

// Callback for when a turnaround should be completed in a holding pattern.
static int64_t hold_callback(alarm_id_t id, void *data) {
    // Get current heading (beginning of the turn)
    oldHeading = aircraft.heading;
    // Set our target heading based on this (with wrap protection)
    targetHeading = (oldHeading + 180);
    if (targetHeading > 360) {
        targetHeading -= 360;
    }
    turnStatus = HOLD_TURN_INPROGRESS;
    return 0; // Tells Pico to not reschedule alarm, we will wait until the turn is over to do that
}

void mode_holdInit() {
    flight_init();
    // We use a vertical guidance PID here so that we can keep the aircraft level; 0deg pitch does not equal 0 altitude change (sadly)
    vertGuid = (PIDController){vertGuid_kP, vertGuid_kI, vertGuid_kD, vertGuid_tau, vertGuid_loLim, vertGuid_upLim, vertGuid_integMin, vertGuid_integMax, vertGuid_kT};
    pid_init(&vertGuid);
    targetAlt = gps.alt; // targetAlt is just the current alt from whenever we enter the mode
    multicore_launch_core1(hold_compute);
}

void mode_hold() {
    flight_update_core0();
    switch (turnStatus) {
        case HOLD_AWAITING_TURN:
            break;
        case HOLD_TURN_BEGUN:
            // Slowly ease into the turn
            if (rollSet <= HOLD_TURN_BANK_ANGLE) {
                rollSet += (HOLD_TURN_BANK_ANGLE * SETPOINT_SMOOTHING_VALUE);
            } else {
                // We've reached the desired angle, now we need to wait for the turn to complete
                turnStatus = HOLD_TURN_INPROGRESS;
                // TODO: possibly set an alarm here just in case something goes wrong and we don't intercept within like 30s; revert to direct?
            }
            break;    
        case HOLD_TURN_INPROGRESS:
            // Wait until it is time to decrease the turn
            if (abs(targetHeading - aircraft.heading) <= HOLD_HEADING_DECREASE_WITHIN) {
                turnStatus = HOLD_TURN_ENDING;
            }
            break;
        case HOLD_TURN_ENDING:
            // Slowly decrease the turn
            if (rollSet >= HOLD_TURN_SLOW_BANK_ANGLE) {
                rollSet -= (HOLD_TURN_BANK_ANGLE * SETPOINT_SMOOTHING_VALUE);
            }
            // Move on to stabilization once we've intercepted the target heading
            if (abs(targetHeading - aircraft.heading) <= HOLD_HEADING_INTERCEPT_WITHIN) {
                turnStatus = HOLD_TURN_STABILIZING;
            }
            break;
        case HOLD_TURN_STABILIZING:
            // Stabilize the turn back to 0 degrees of bank, then mark it as completed (unscheduled)
            if (rollSet >= 0) {
                rollSet -= (HOLD_TURN_BANK_ANGLE * SETPOINT_SMOOTHING_VALUE);
            }
            break;
        case HOLD_TURN_UNSCHEDULED:
            if (add_alarm_in_ms((HOLD_TIME_PER_LEG_S * 1000), hold_callback, NULL, true) >= 0) {
                turnStatus = HOLD_AWAITING_TURN;
            }
            break;
    }
}

#endif // WIFLY_ENABLED

void mode_holdDeinit() {
    multicore_reset_core1();
}
