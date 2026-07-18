/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <math.h>
#include "platform/helpers.h"
#include "platform/time.h"
#include "platform/types.h"

#include "ctrl/control.h"
#include "ctrl/flight.h"
#include "ctrl/throttle.h"
#include "io/gps.h"
#include "lib/pid.h"
#include "modes/auto.h"
#include "sys/configuration.h"
#include "sys/log.h"

#include "hold.h"

// The amount of time (in seconds) that the aircraft will fly straight for in the holding pattern, before turning back
// around 180 degrees
#define HOLD_TIME_PER_LEG_S 30

// The bank angle to turn at when making a 180 in the holding pattern--needs to be positive!
#define HOLD_TURN_BANK_ANGLE 20
// The bank angle to turn at the end of the turn when we are about to intercept the heading--also needs to be positive!
#define HOLD_TURN_SLOW_BANK_ANGLE 5

// The value (in degrees) within which the bank angle will begin to be decreased
#define HOLD_HEADING_DECREASE_WITHIN 10

// The value (in degrees) within which the heading will be considered intercepted
#define HOLD_HEADING_INTERCEPT_WITHIN 2

// The value (in degrees) within which the commanded roll is considered to have converged on its target bank angle
#define HOLD_ROLL_CONVERGED_WITHIN 1.5

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

// rollTarget is the bank angle we're currently working towards; rollOut/pitchOut are the smoothed,
// actually-commanded values sent to flight_update() (mirrors the pattern used in auto.c)
static f32 rollTarget;
static f32 rollOut, pitchOut;

static PIDController vertGuid;

// Callback for when a turnaround should be completed in a holding pattern
static i32 turn_around(void *data) {
    // Get current track (beginning of the turn)
    oldTrack = gps.track;
    // Set our target heading based on this (with wrap protection)
    targetTrack = (oldTrack + 180);
    if (targetTrack > 360) {
        targetTrack -= 360;
    }
    turnStatus = HOLD_TURN_BEGUN;
    return 0; // Don't reschedule, we will wait until the turn is complete to do that
    (void)data;
}

bool hold_init() {
    // flight_init() and throttle_init() are intentionally not called here
    // Hold mode is only ever entered from auto mode, which already has flight and speed modes active and correctly
    // tracking the last waypoint's target speed

    // We use a vertical guidance PID here so that we can keep the aircraft level; 0deg pitch does not equal 0 altitude
    // change (sadly)
    vertGuid = (PIDController){
        .kp = VERTGD_KP,
        .ki = VERTGD_KI,
        .kd = VERTGD_KD,
        .tau = calibration.pid.tau,
        .limMin = VERTGD_LIM_MIN,
        .limMax = VERTGD_LIM_MAX,
    };
    pid_init(&vertGuid);
    turnStatus = HOLD_TURN_UNSCHEDULED;
    rollTarget = 0.f;
    rollOut = 0.f;
    pitchOut = 0.f;
    targetAlt = gps.alt; // targetAlt is just the current alt from whenever we enter the mode
    return true;
}

void hold_update() {
    pid_update(&vertGuid, targetAlt, gps.alt);
    // Smooth the commanded roll and pitch as done in auto.c
    rollOut = lerp(rollOut, rollTarget, GUIDANCE_SMOOTHING);
    pitchOut = lerp(pitchOut, (f32)vertGuid.out, GUIDANCE_SMOOTHING);
    flight_update(rollOut, pitchOut, 0, false);
    throttle_update();

    switch (turnStatus) {
        case HOLD_TURN_BEGUN:
            rollTarget = HOLD_TURN_BANK_ANGLE;
            // Once the actual commanded roll has caught up to the target bank, the turn is properly established
            if (fabsf(rollOut - HOLD_TURN_BANK_ANGLE) < HOLD_ROLL_CONVERGED_WITHIN) {
                turnStatus = HOLD_TURN_INPROGRESS;
            }
            break;
        case HOLD_TURN_INPROGRESS:
            // Wait until it is time to decrease the turn
            if (fabsf(control_get_heading_diff(targetTrack, gps.track)) <= HOLD_HEADING_DECREASE_WITHIN) {
                turnStatus = HOLD_TURN_ENDING;
            }
            break;
        case HOLD_TURN_ENDING:
            rollTarget = HOLD_TURN_SLOW_BANK_ANGLE;
            // Move on to stabilization once we've intercepted the target heading
            if (fabsf(control_get_heading_diff(targetTrack, gps.track)) <= HOLD_HEADING_INTERCEPT_WITHIN) {
                turnStatus = HOLD_TURN_STABILIZING;
            }
            break;
        case HOLD_TURN_STABILIZING:
            rollTarget = 0.f;
            // Wait for the commanded roll to settle back to wings-level before considering the turn complete
            if (fabsf(rollOut) < HOLD_ROLL_CONVERGED_WITHIN) {
                turnStatus = HOLD_TURN_UNSCHEDULED;
            }
            break;
        case HOLD_TURN_UNSCHEDULED:
            callback_in_ms((HOLD_TIME_PER_LEG_S * 1000), turn_around, NULL);
            turnStatus = HOLD_AWAITING_TURN;
            break;
        default:
            break;
    }
}
