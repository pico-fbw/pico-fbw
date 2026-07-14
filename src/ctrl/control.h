#pragma once

#include "platform/types.h"

typedef enum Axis {
    AXIS_ROLL,
    AXIS_PITCH,
} Axis;

typedef enum Elevon {
    ELEVON_LEFT,
    ELEVON_RIGHT,
} Elevon;

/**
 * Gets the degrees per second for the given axis.
 * @param axis the axis to get the degrees per second for
 * @param roll roll input from -90 to 90 degrees
 * @param pitch pitch input from -90 to 90 degrees
 * @return the degrees per second for the given axis
 */
f32 control_get_dps(Axis axis, f32 roll, f32 pitch);

/**
 * Calculates the axis adjustments for the given inputs.
 * @param axis the axis to calculate the adjustment for
 * @param roll roll input from -90 to 90 degrees
 * @param pitch pitch input from -90 to 90 degrees
 * @return the adjustment to be added to the current setpoint
 * @note This function is time-based and should be called at a regular interval to get the correct adjustment.
 * The value it returns should be added to the current setpoints, and over time will accumulate at the requested rate.
 */
f32 control_calc_adjust(Axis axis, f32 roll, f32 pitch);

/**
 * Resets the control calculations.
 * @note This should be called when the control loop is reset or when the control mode changes;
 * basically whenever calculations won't be run for a while.
 */
void control_reset();

/**
 * Performs mixing calculations for an elevon (to be used in flying wing control modes).
 * @param elevon the elevon to calculate the mixing for
 * @param roll roll input from 0 to 180 degrees
 * @param pitch pitch input from 0 to 180 degrees
 * @return the mixing value for the elevon from 0 to 180 degrees
 */
f32 control_mix_elevon(Elevon elevon, f64 roll, f64 pitch);

/**
 * @return the shortest signed difference between the target and track headings, in degrees.
 * @param target the target heading in degrees (0-360)
 * @param track the currently tracked heading in degrees (0-360)
 * @note The result will be in the range [-180, 180].
 */
f32 control_get_heading_diff(f32 target, f32 track);

/**
 * Applies an exponential (cubic) curve to a linear input.
 * @param linear linear input [0, 180]
 * @return the cubic-mapped value
 */
f32 control_apply_expo(f32 linear);
