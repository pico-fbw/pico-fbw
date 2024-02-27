#pragma once

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
 * If override is set to false and roll is past the deadband, a "static damper" will be applied to the rudder (current aileron degree * CONTROL_RUDDER_SENSITIVITY).
 * If override is set to false and roll is below the deadband, a "dynamic damper" will be applied instead (uses PID to achieve the correct yaw angle).
*/
void flight_update(double roll, double pitch, double yaw, bool override);
