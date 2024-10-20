#pragma once

#include <math.h>
#include <stdbool.h>
#include "platform/types.h"

/* Madgwick filter structure. */
typedef struct Madgwick {
    f32 beta;
    f32 q0;
    f32 q1;
    f32 q2;
    f32 q3;
    f32 freq;
    f32 inv_freq;
    u32 counter;
} Madgwick;

/* Create a new filter and initialize it by resetting the Quaternion and setting
 * the update rate to 100Hz and the gain to 0.1
 * Returns a pointer to a `Madgwick`, or NULL otherwise.
 */
Madgwick *madgwick_create(void);

/* Clean up and return memory for the filter
 */
bool madgwick_destroy(Madgwick **filter);

/* Sets the filter update rate and gain (defaults to freq=100Hz and gain=0.1)
 * The `madgwick_update()` function then expects to be called at `freq`
 * per second.
 */
bool madgwick_set_params(Madgwick *filter, f32 frequency, f32 beta);

/* Resets the filter Quaternion to an initial state (of {1,0,0,0}).
 */
bool madgwick_reset(Madgwick *filter);

/* Run an update cycle on the filter. Inputs gx/gy/gz are in any calibrated input (for example,
 * m/s/s or G), inputs of ax/ay/az are in Rads/sec, inputs of mx/my/mz are in any calibrated
 * input (for example, uTesla or Gauss). The inputs of mx/my/mz can be passed as 0.0, in which
 * case the magnetometer fusion will not occur.
 * Returns true on success, false on failure.
 */
bool madgwick_update(Madgwick *filter, f32 gx, f32 gy, f32 gz, f32 ax, f32 ay, f32 az, f32 mx, f32 my, f32 mz);

/*
 * Returns AHRS Quaternion, as values between -1.0 and +1.0.
 * Each of q0, q1, q2, q3 pointers may be NULL, in which case they will not be
 * filled in.
 * Returns true on success, false in case of error, in which case the values of
 * q0, q1, q2 and q3 are undetermined.
 */
bool madgwick_get_quaternion(Madgwick *filter, f32 *q0, f32 *q1, f32 *q2, f32 *q3);

/*
 * Returns AHRS angles of roll, pitch and yaw, in Radians between -Pi and +Pi.
 * Each of the roll, pitch and yaw pointers may be NULL, in which case they will
 * not be filled in.
 * Returns true on success, false in case of error, in which case the values of
 * roll, pitch and yaw are undetermined.
 */
bool madgwick_get_angles(Madgwick *filter, f32 *roll, f32 *pitch, f32 *yaw);

/*
 * Returns filter counter. Each call to `madgwick_update()` increments the
 * counter by one.
 * Returns true on success, false in case of error, in which case the value of
 * counter is undetermined.
 */
bool madgwick_get_counter(Madgwick *filter, uint32_t *counter);
