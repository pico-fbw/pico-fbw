#ifndef __AUTO_H
#define __AUTO_H

/* PID constants for the autopilot's lateral guidance. */
#define latGuid_kP 0.005
#define latGuid_kI 0.008
#define latGuid_kD 0.002
#define latGuid_tau 0.001
#define latGuid_lim 33 // The maximum roll angle the autopilot can command
#define latGuid_integMin -50.0
#define latGuid_integMax 50.0
#define latGuid_kT 0.01

/* PID constants for the autopilot's vertical guidance. */
#define vertGuid_kP 0.05
#define vertGuid_kI 0.0025
#define vertGuid_kD 0.001
#define vertGuid_tau 0.001
#define vertGuid_loLim -15 // The minimum pitch angle the autopilot can command
#define vertGuid_upLim 25 // The maximum pitch angle the autopilot can command
#define vertGuid_integMin -50.0
#define vertGuid_integMax 50.0
#define vertGuid_kT 0.01

#define INTERCEPT_RADIUS 25 // The radius at which to consider a waypoint "incercepted" in meters
// TODO: do I need to change this for different speeds? idk if it will make too much of a difference, remember what aviation simmer said

/**
 * Initializes auto mode.
 * @return true if initialization was successful, false if not
*/
bool mode_autoInit();

/**
 * Executes one cycle of auto mode.
*/
void mode_auto();

#endif // __AUTO_H
