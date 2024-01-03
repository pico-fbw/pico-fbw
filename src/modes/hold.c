/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "pico/time.h"

#include "../io/flash.h"

#include "../lib/pid.h"

#include "../sys/log.h"
#include "../sys/throttle.h"

#include "auto.h"
#include "flight.h"

#include "hold.h"

static uint8_t turnStatus = HOLD_TURN_UNSCHEDULED;

static float oldTrack;
static float targetTrack;
static int targetAlt;

static double rollSet;

static PIDController vertGuid;

// Callback for when a turnaround should be completed in a holding pattern.
static int64_t turnAround(alarm_id_t id, void *data) {
    // Get current track (beginning of the turn)
    oldTrack = gps.track;
    // Set our target heading based on this (with wrap protection)
    targetTrack = (oldTrack + 180);
    if (targetTrack > 360) {
        targetTrack -= 360;
    }
    turnStatus = HOLD_TURN_INPROGRESS;
    return 0; // Tells Pico to not reschedule alarm, we will wait until the turn is complete to do that
}

bool hold_init() {
    flight_init();
    throttle.init();
    if (throttle.supportedMode < THRMODE_SPEED) {
        log_message(WARNING, "SPEED mode required!", 2000, 0, false);
        return false;
    }
    throttle.mode = THRMODE_SPEED;
    // We try to maintain the speed of the aircraft as it was entering the holding pattern
    throttle.target = gps.speed;
    // We use a vertical guidance PID here so that we can keep the aircraft level; 0deg pitch does not equal 0 altitude change (sadly)
    vertGuid = (PIDController){vertGuid_kP, vertGuid_kI, vertGuid_kD, vertGuid_tau, vertGuid_loLim,
    vertGuid_upLim, vertGuid_integMin, vertGuid_integMax, vertGuid_kT};
    pid_init(&vertGuid);
    targetAlt = gps.alt; // targetAlt is just the current alt from whenever we enter the mode
    return true;
}

void hold_update() {
    pid_update(&vertGuid, targetAlt, gps.alt);
    flight_update(rollSet, vertGuid.out, 0, false);
    throttle.update();

    switch (turnStatus) {
        case HOLD_TURN_BEGUN:
            // Slowly ease into the turn
            if (rollSet <= HOLD_TURN_BANK_ANGLE) {
                rollSet += (HOLD_TURN_BANK_ANGLE * flash.control[CONTROL_RUDDER_SENSITIVITY]);
            } else {
                // We've reached the desired angle, now we need to wait for the turn to complete
                turnStatus = HOLD_TURN_INPROGRESS;
            }
            break;    
        case HOLD_TURN_INPROGRESS:
            // Wait until it is time to decrease the turn
            if (fabsf(targetTrack - gps.track) <= HOLD_HEADING_DECREASE_WITHIN) {
                turnStatus = HOLD_TURN_ENDING;
            }
            break;
        case HOLD_TURN_ENDING:
            // Slowly decrease the turn
            if (rollSet >= HOLD_TURN_SLOW_BANK_ANGLE) {
                rollSet -= (HOLD_TURN_BANK_ANGLE * flash.control[CONTROL_RUDDER_SENSITIVITY]);
            }
            // Move on to stabilization once we've intercepted the target heading
            if (fabsf(targetTrack - gps.track) <= HOLD_HEADING_INTERCEPT_WITHIN) {
                turnStatus = HOLD_TURN_STABILIZING;
            }
            break;
        case HOLD_TURN_STABILIZING:
            // Stabilize the turn back to 0 degrees of bank, then mark it as completed (unscheduled)
            if (rollSet >= 0) {
                rollSet -= (HOLD_TURN_BANK_ANGLE * flash.control[CONTROL_RUDDER_SENSITIVITY]);
            }
            break;
        case HOLD_TURN_UNSCHEDULED:
            if (add_alarm_in_ms((HOLD_TIME_PER_LEG_S * 1000), turnAround, NULL, true) >= 0) {
                turnStatus = HOLD_AWAITING_TURN;
            }
            break;
        default:
            break;
    }
}
