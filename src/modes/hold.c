/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <math.h>
#include "platform/time.h"
#include "platform/types.h"

#include "io/gps.h"

#include "lib/pid.h"

#include "sys/configuration.h"
#include "sys/log.h"
#include "sys/throttle.h"

#include "modes/auto.h"
#include "modes/flight.h"

#include "hold.h"

// The amount of time (in seconds) that the aircraft will fly straight for in the holding pattern, before turning back around
// 180 degrees.
#define HOLD_TIME_PER_LEG_S 30

// The bank angle to turn at when making a 180 in the holding pattern--needs to be positive!
#define HOLD_TURN_BANK_ANGLE 25
// The bank angle to turn at the end of the turn when we are about to intercept the heading--also needs to be positive!
#define HOLD_TURN_SLOW_BANK_ANGLE 5

// The value (in degrees) within which the bank angle will begin to be decreased.
#define HOLD_HEADING_DECREASE_WITHIN 10

// The value (in degrees) within which the heading will be considered intercepted.
#define HOLD_HEADING_INTERCEPT_WITHIN 2

typedef enum HoldStatus {
    HOLD_AWAITING_TURN,
    HOLD_TURN_BEGUN,
    HOLD_TURN_INPROGRESS,
    HOLD_TURN_ENDING,
    HOLD_TURN_STABILIZING,
    HOLD_TURN_UNSCHEDULED,
} HoldStatus;

static HoldStatus turnStatus = HOLD_TURN_UNSCHEDULED;

static f32 oldTrack;
static f32 targetTrack;
static i32 targetAlt;

static f64 rollSet;

static PIDController vertGuid;

// Callback for when a turnaround should be completed in a holding pattern
static i32 turn_around() {
    // Get current track (beginning of the turn)
    oldTrack = gps.track;
    // Set our target heading based on this (with wrap protection)
    targetTrack = (oldTrack + 180);
    if (targetTrack > 360) {
        targetTrack -= 360;
    }
    turnStatus = HOLD_TURN_INPROGRESS;
    return 0; // Don't reschedule, we will wait until the turn is complete to do that
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
// We use a vertical guidance PID here so that we can keep the aircraft level; 0deg pitch does not equal 0 altitude change
// (sadly)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    vertGuid = (PIDController){vertGuid_kP,    vertGuid_kI,    vertGuid_kD,        vertGuid_tau,
                               vertGuid_loLim, vertGuid_hiLim, -vertGuid_integLim, vertGuid_integLim};
#pragma GCC diagnostic pop
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
                rollSet += (HOLD_TURN_BANK_ANGLE * config.control[CONTROL_RUDDER_SENSITIVITY]);
            } else {
                // We've reached the desired angle, now we need to wait for the turn to complete
                turnStatus = HOLD_TURN_INPROGRESS;
            }
            break;
        case HOLD_TURN_INPROGRESS:
            // Wait until it is time to decrease the turn
            if (fabsf(targetTrack - gps.track) <= HOLD_HEADING_DECREASE_WITHIN)
                turnStatus = HOLD_TURN_ENDING;
            break;
        case HOLD_TURN_ENDING:
            // Slowly decrease the turn
            if (rollSet >= HOLD_TURN_SLOW_BANK_ANGLE)
                rollSet -= (HOLD_TURN_BANK_ANGLE * config.control[CONTROL_RUDDER_SENSITIVITY]);
            // Move on to stabilization once we've intercepted the target heading
            if (fabsf(targetTrack - gps.track) <= HOLD_HEADING_INTERCEPT_WITHIN) {
                turnStatus = HOLD_TURN_STABILIZING;
            }
            break;
        case HOLD_TURN_STABILIZING:
            // Stabilize the turn back to 0 degrees of bank, then mark it as completed (unscheduled)
            if (rollSet >= 0)
                rollSet -= (HOLD_TURN_BANK_ANGLE * config.control[CONTROL_RUDDER_SENSITIVITY]);
            break;
        case HOLD_TURN_UNSCHEDULED:
            callback_in_ms((HOLD_TIME_PER_LEG_S * 1000), turn_around);
            turnStatus = HOLD_AWAITING_TURN;
            break;
        default:
            break;
    }
}
