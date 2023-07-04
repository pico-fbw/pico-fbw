#ifndef flight_h
#define flight_h

#include "../io/imu.h"

// Contains the aircraft's inertial angles, will be updated whenever flight_update_core0() is called.
extern inertialAngles aircraft;

/**
 * Initializes the flight system (axis PIDs).
*/
void flight_init();

/**
 * Updates the flight system's aircraft data (from IMU), checks flight envelope, and actuates servos to the current commanded flight angles (from PIDs).
 * ONLY call this function from CORE 0!!
 * Must be called periodically to ensure up-to-date IMU data and servo actuation.
*/
void flight_update_core0();

/**
 * Updates the flight system's setpoints and computes necessary PID values.
 * ONLY call this function from CORE 1!!
 * Must be called periodically to ensure up-to-date PID calculations.
 * @param rollSetpoint The desired roll angle.
 * @param pitchSetpoint The desired pitch angle.
 * @param yawSetpoint The desired yaw angle (only applicable if override is enabled).
 * @param yawOverride Whether or not the yaw angle should be overridden to yawSetpoint.
 * 
 * The yaw damper functionality may be a bit confusing, I'll explain it here.
 * If yawOverride is set to true, the value from yawSetpoint will be passed directly to the rudder.
 * If yawOverride is set to false and rollSetpoint is past the deadband, a "static damper" will be applied to the rudder (current aileron degree * RUDDER_TURNING_VALUE).
 * If yawOverride is set to false and rollSetpoint is below the deadband, a "dynamic damper" will be applied instead (uses PID to achieve the correct yaw angle).
*/
void flight_update_core1(double rollSetpoint, double pitchSetpoint, double yawSetpoint, bool yawOverride);

#endif // flight_h
