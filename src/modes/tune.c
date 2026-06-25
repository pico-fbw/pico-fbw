/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <math.h>
#include "platform/time.h"

#include "ctrl/aircraft.h"
#include "ctrl/control.h"
#include "ctrl/flight.h"
#include "io/imu.h"
#include "io/receiver.h"
#include "modes/normal.h"
#include "sys/configuration.h"
#include "sys/log.h"
#include "sys/print.h"

#include "tune.h"

// The difference between the resquested and actual axis travel rates that can trigger a possible P gain increase
#define P_GAIN_DIFF_THRESHOLD 1.f
// The time (in milliseconds) that P_GAIN_DIFF_THRESHOLD must be exceeded to trigger a P gain increase
#define P_GAIN_DIFF_TIME_MS 500
// The amount to increase/decrease the P gain by
#define P_GAIN_STEP 0.25f
#define P_GAIN_MAX 6.f

// The amount of overshoot past the setpoint required to trigger a D gain increase
#define D_GAIN_OVERSHOOT_THRESHOLD 4.f
// The amount to increase/decrease the D gain by
#define D_GAIN_STEP 0.1f
#define D_GAIN_MAX 12.f

// Minimum time between gain updates for a single axis to prevent runaway tuning
#define GAIN_UPDATE_COOLDOWN_MS 1000

// If this amount of time passes without any tune events, the system is considered tuned
#define TUNED_THRESHOLD_MS 30E3

static Timestamp lastTuneEvent;
static u32 tDiffRoll = 0, tDiffPitch = 0;
static u32 tLastUpdateRoll = 0, tLastUpdatePitch = 0;

/**
 * Updates the P and D gains for the given axis, if required.
 * @param axis axis to update the gains for
 * @param req_rate requested rate of the axis in deg/s
 * @param act_rate actual rate of the axis in deg/s
 * @param setpoint the setpoint for the axis
 * @param act_angle the actual angle of the axis
 */
static void update_gain(Axis axis, f32 req_rate, f32 act_rate, f32 setpoint, f32 act_angle) {
    u32 *tDiff       = (axis == AXIS_ROLL) ? &tDiffRoll       : &tDiffPitch;
    u32 *tLastUpdate = (axis == AXIS_ROLL) ? &tLastUpdateRoll  : &tLastUpdatePitch;

    u32 now = time_ms();
    if (*tLastUpdate != 0 && now - *tLastUpdate < GAIN_UPDATE_COOLDOWN_MS) {
        return; // Update rate throttled
    }

    // Scale the P threshold to 15% of the requested rate, with a minimum floor to prevent
    // sensor noise from keeping the threshold permanently exceeded at low stick inputs
    f32 pGainThreshold = fmaxf(P_GAIN_DIFF_THRESHOLD, fabsf(req_rate) * 0.15f);
    f32 rateError = fabsf(req_rate - act_rate);

    // If the response is too slow, increase P gain
    if (rateError > pGainThreshold) {
        if (*tDiff == 0) {
            *tDiff = now;
        }
        if (now - *tDiff > P_GAIN_DIFF_TIME_MS) {
            f64 kP;
            flight_tunings_get(axis, &kP, NULL, NULL);
            kP += P_GAIN_STEP;
            if (kP > P_GAIN_MAX) {
                *tDiff = 0;
                return;
            }
            flight_tunings_update(axis, kP, INFINITY, INFINITY, false);
            *tLastUpdate = now;
            *tDiff = 0;
            lastTuneEvent = timestamp_now();
        }
        return; // Don't evaluate D while still building P response
    }

    // Error is small, reset P debounce
    *tDiff = 0;

    // Only check for overshoot if there's a meaningful setpoint; this avoids bumping kD
    // due to IMU noise and natural attitude offset when the plane is just holding level
    if (fabsf(setpoint) < 1.f && fabsf(req_rate) < P_GAIN_DIFF_THRESHOLD) {
        return;
    }

    // Check for attitude overshoot past the setpoint to determine if D needs increasing
    // Overshoot = actual angle has crossed to the opposite side of the setpoint
    f32 attitudeError = act_angle - setpoint;
    if (fabsf(attitudeError) > D_GAIN_OVERSHOOT_THRESHOLD) {
        f64 kD;
        flight_tunings_get(axis, NULL, NULL, &kD);
        kD += D_GAIN_STEP;
        if (kD > D_GAIN_MAX) {
            return;
        }
        flight_tunings_update(axis, INFINITY, INFINITY, kD, false);
        *tLastUpdate = now;
        lastTuneEvent = timestamp_now();
    }
}

void tune_init() {
    // Tune depends on normal mode
    normal_init();
    lastTuneEvent = timestamp_now();
    tDiffRoll = 0;
    tDiffPitch = 0;
    tLastUpdateRoll = 0;
    tLastUpdatePitch = 0;
}

void tune_update() {
    normal_update();
    if (tune_is_tuned()) {
        return;
    }

    // Get the current inputs and use them to calculate the mapped ("requested") rates in dps
    f32 rollInput = receiver_get((i16)config.pins[PINS_INPUT_AIL], RECEIVER_MODE_DEGREE) - 90.f;
    f32 pitchInput = receiver_get((i16)config.pins[PINS_INPUT_ELE], RECEIVER_MODE_DEGREE) - 90.f;
    f32 reqRollRate = control_get_dps(AXIS_ROLL, rollInput, pitchInput);
    f32 reqPitchRate = control_get_dps(AXIS_PITCH, rollInput, pitchInput);
    // Get current attitude setpoints from normal mode (to check for overshoot)
    f32 rollSet, pitchSet;
    normal_get(&rollSet, &pitchSet);

    // Update the gains for roll and pitch axes
    // Only tune when the pilot is actively commanding movement
    if (fabsf(reqRollRate) > P_GAIN_DIFF_THRESHOLD) {
        update_gain(AXIS_ROLL, reqRollRate, imu.rollRate, rollSet, imu.roll);
    }
    if (fabsf(reqPitchRate) > P_GAIN_DIFF_THRESHOLD) {
        update_gain(AXIS_PITCH, reqPitchRate, imu.pitchRate, pitchSet, imu.pitch);
    }

    // Set the tuned flag if there haven't been any tune events for a while
    if (time_since_ms(&lastTuneEvent) > TUNED_THRESHOLD_MS) {
        calibration.pid[PID_TUNED] = true;
        config_save();
        printsys(aircraft, "tuning complete");
    }
}

void tune_deinit() {
    normal_deinit();
}

bool tune_is_tuned() {
    return (bool)calibration.pid[PID_TUNED];
}
