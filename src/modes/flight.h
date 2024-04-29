#pragma once

#include <stdbool.h>
#include "platform/types.h"

#include "sys/control.h"

/**
 * Initializes the flight system (axis PIDs).
 */
void flight_init();

/**
 * Updates the flight system's aircraft data (from IMU and GPS), checks flight envelope,
 * computes PID, updates setpoints, and actuates servos to the current commanded flight angles (from PIDs).
 * ONLY call this function from CORE 0!!
 * Must be called periodically to ensure up-to-date flight data, setpoints and servo actuation.
 *
 * @param roll the desired roll angle
 * @param pitch the desired pitch angle
 * @param yaw the desired yaw angle (only applicable if override is enabled)
 * @param override whether or not the yaw angle should be overridden to yawSetpoint
 *
 * @note
 * The yaw damper functionality may be a bit confusing, I'll explain it here.
 * If override is set to true, the value from yaw will be passed directly to the rudder.
 * If override is set to false and roll is past the deadband, a "static damper" will be applied to the rudder (current aileron
 * degree * CONTROL_RUDDER_SENSITIVITY). If override is set to false and roll is below the deadband, a "dynamic damper" will be
 * applied instead (uses PID to achieve the correct yaw angle).
 */
void flight_update(f64 roll, f64 pitch, f64 yaw, bool override);

/**
 * Gets the current PID parameters for an axis.
 * @param axis the axis to get the PID parameters for
 * @param kP pointer to where to store the proportional gain, or NULL if not needed
 * @param kI pointer to where to store the integral gain, or NULL if not needed
 * @param kD pointer to where to store the derivative gain, or NULL if not needed
 */
void flight_params_get(Axis axis, f64 *kP, f64 *kI, f64 *kD);

/**
 * Updates an axis's PID parameters.
 * @param axis the axis to update the PID parameters for
 * @param kP the new proportional gain, or INFINITY to keep the current value
 * @param kI the new integral gain, or INFINITY to keep the current value
 * @param kD the new derivative gain, or INFINITY to keep the current value
 * @param reset whether or not to reset the PID
 */
void flight_params_update(Axis axis, f64 kP, f64 kI, f64 kD, bool reset);
