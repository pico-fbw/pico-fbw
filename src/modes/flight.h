#ifndef __FLIGHT_H
#define __FLIGHT_H

#include "../io/imu.h"
#include "../io/gps.h"

#include "../config.h"

// Contains the aircraft's inertial angles, will be updated whenever flight_update_core0() is called.
extern Angles aircraft;
#ifdef GPS_ENABLED
    // Contains the aircraft's GPS positioning data, will be updated whenever flight_update_core0() is called.
    extern GPS gps;
#endif

/**
 * Initializes the flight system (axis PIDs).
*/
void flight_init();

/**
 * Updates the flight system's aircraft data (from IMU and GPS), checks flight envelope, computes PID, updates setpoints, and actuates servos to the current commanded flight angles (from PIDs).
 * ONLY call this function from CORE 0!!
 * Must be called periodically to ensure up-to-date flight data, setpoints and servo actuation.
 * 
 * @param roll The desired roll angle.
 * @param pitch The desired pitch angle.
 * @param yaw The desired yaw angle (only applicable if override is enabled).
 * @param override Whether or not the yaw angle should be overridden to yawSetpoint.
 * 
 * @note
 * The yaw damper functionality may be a bit confusing, I'll explain it here.
 * If override is set to true, the value from yaw will be passed directly to the rudder.
 * If override is set to false and roll is past the deadband, a "static damper" will be applied to the rudder (current aileron degree * RUD_TURN_SENSITIVITY).
 * If override is set to false and roll is below the deadband, a "dynamic damper" will be applied instead (uses PID to achieve the correct yaw angle).
*/
void flight_update(double roll, double pitch, double yaw, bool override);

#endif // __FLIGHT_H
