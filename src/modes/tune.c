/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <math.h>
#include "platform/time.h"

#include "io/aahrs.h"
#include "io/receiver.h"

#include "modes/flight.h"
#include "modes/normal.h"

#include "sys/configuration.h"
#include "sys/control.h"

#include "tune.h"

// The difference (in deg) between the resquested and actual axis travel rates that can trigger a possible P gain increase
#define P_GAIN_DIFF_THRESHOLD 1.f
// The time (in milliseconds) that P_GAIN_DIFF_THRESHOLD must be exceeded to trigger a P gain increase
#define P_GAIN_DIFF_TIME_MS 250
// The amount to increase/decrease the P gain by
#define P_GAIN_STEP 0.1f

// The difference (in deg) between the resquested and actual axis travel rates that can trigger a possible D gain increase
#define D_GAIN_REQ_RATE_THRESHOLD 1.f
// The amount of overshoot (in deg) that must be exceeded (after D_GAIN_REQ_RATE_THRESHOLD is met) to trigger a D gain increase
#define D_GAIN_OVERSHOOT_THRESHOLD 0.5f
// The amount to increase/decrease the D gain by
#define D_GAIN_STEP 0.1f

// If this amount of time passes without any tune events, the system is considered tuned
#define TUNED_THRESHOLD_MS 10000

static Timestamp lastTuneEvent;

/**
 * Updates the P and D gains for the given axis, if required.
 * @param axis axis to update the gains for
 * @param req_rate requested rate of the axis in deg/s
 * @param act_rate actual rate of the axis in deg/s
 */
static void update_gain(Axis axis, f32 req_rate, f32 act_rate) {
    static u32 tDiffRoll = 0;
    static u32 tDiffPitch = 0;
    u32 *tDiff = (axis == AXIS_ROLL) ? &tDiffRoll : &tDiffPitch;

    // If the difference between the requested and actual rates is greater than the threshold for longer than the set time,
    // increase the P gain
    if (fabsf(req_rate - act_rate) > P_GAIN_DIFF_THRESHOLD) {
        if (*tDiff == 0)
            *tDiff = time_ms();
        if (time_ms() - *tDiff > P_GAIN_DIFF_TIME_MS) {
            f64 kP;
            flight_params_get(axis, &kP, NULL, NULL);
            flight_params_update(axis, kP + P_GAIN_STEP, INFINITY, INFINITY, false);
            *tDiff = 0;
        }
        lastTuneEvent = timestamp_now();
        // Check if any overshoots have occured and increase the D gain if necessary
    } else if (fabsf(req_rate - act_rate) < D_GAIN_REQ_RATE_THRESHOLD) {
        if (fabsf(req_rate - act_rate) > D_GAIN_OVERSHOOT_THRESHOLD) {
            f64 kD;
            flight_params_get(axis, NULL, &kD, NULL);
            flight_params_update(axis, INFINITY, kD + D_GAIN_STEP, INFINITY, false);
            lastTuneEvent = timestamp_now();
        }
    }
}

void tune_init() {
    // Tune depends on normal mode
    normal_init();
}

void tune_update() {
    normal_update();
    if (calibration.pid[PID_TUNED])
        return;

    f32 rollInput = receiver_get((u32)config.pins[PINS_INPUT_AIL], RECEIVER_MODE_DEGREE) - 90.f;
    f32 pitchInput = receiver_get((u32)config.pins[PINS_INPUT_ELE], RECEIVER_MODE_DEGREE) - 90.f;
    // Get the requested and actual roll and pitch rates
    f32 reqRollRate = control_get_dps(AXIS_ROLL, rollInput, pitchInput);
    f32 reqPitchRate = control_get_dps(AXIS_PITCH, rollInput, pitchInput);
    f32 actRollRate = aahrs.rollRate;
    f32 actPitchRate = aahrs.pitchRate;

    update_gain(AXIS_ROLL, reqRollRate, actRollRate);
    update_gain(AXIS_PITCH, reqPitchRate, actPitchRate);

    // Set the tuned flag if there haven't been any tune events for a while
    if (time_since_ms(&lastTuneEvent) > TUNED_THRESHOLD_MS) {
        calibration.pid[PID_TUNED] = true;
        config_save();
    }
}

void tune_deinit() {
    normal_deinit();
}

bool tune_is_tuned() {
    return (bool)calibration.pid[PID_TUNED];
}
