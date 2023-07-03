#ifndef flight_h
#define flight_h

/**
 * Initializes the flight system (axis PIDs).
*/
void flight_init();

/**
 * Updates the flight system's axis input data.
 * @param rollSetpoint The desired roll angle.
 * @param rollAngle The current roll angle.
 * @param pitchSetpoint The desired pitch angle.
 * @param pitchAngle The current pitch angle.
 * @param yawSetpoint The desired yaw angle (only applicable if override is enabled).
 * @param yawAngle The current yaw angle.
 * @param yawOverride Whether or not the yaw angle should be overridden to yawSetpoint.
 * 
 * The yaw damper functionality may be a bit confusing, I'll explain it here.
 * If yawOverride is set to true, the value from yawSetpoint will be passed directly to the rudder.
 * If yawOverride is set to false and rollSetpoint is past the deadband, a "static damper" will be applied to the rudder (current aileron degree * RUDDER_TURNING_VALUE).
 * If yawOverride is set to false and rollSetpoint is below the deadband, a "dynamic damper" will be applied instead (uses PID to achieve the correct yaw angle).
*/
void flight_update(double rollSetpoint, double rollAngle, double pitchSetpoint, double pitchAngle, double yawSetpoint, double yawAngle, bool yawOverride);

/**
 * @return The current roll angle commanded by the roll PID controller.
*/
double flight_getRollOut();

/**
 * @return The current pitch angle commanded by the pitch PID controller.
*/
double flight_getPitchOut();

/**
 * Checks the current flight envelope of the aircraft.
 * If the flight envelope is unsafe, the IMU will be deemed unsafe and the system will perform necessary control law degredation.
 * @param rollAngle The current roll angle of the aircraft.
 * @param pitchAngle The current pitch angle of the aircraft.
 * @return true if the flight envelope is safe, false if not
*/
bool flight_checkEnvelope(float rollAngle, float pitchAngle);

#endif // flight_h
