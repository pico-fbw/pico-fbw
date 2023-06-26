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
 * @param yawSetpoint The desired yaw angle.
 * @param yawAngle The current yaw angle.
 * @param yawDampEnabled Whether the yaw damper is enabled.
*/
void flight_update(double rollSetpoint, double rollAngle, double pitchSetpoint, double pitchAngle, double yawSetpoint, double yawAngle, bool yawDampEnabled);

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
