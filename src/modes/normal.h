#ifndef __NORMAL_H
#define __NORMAL_H

/**
 * Initializes normal mode.
*/
void mode_normalInit();

/**
 * Executes one cycle of normal mode.
*/
void mode_normal();

/**
 * Fully de-initializes normal mode.
*/
void mode_normalDeinit();

/**
 * Manually adjust the setpoints of normal mode.
 * @param roll the roll setpoint
 * @param pitch the pitch setpoint
 * @param yaw the yaw setpoint
 * @param throttle the throttle setpoint
 * @param useThrottle whether or not to use the throttle setpoint
 * @return true if the setpoints were adjusted, false if it was prevented by the user (already manually inputting).
*/
bool mode_normalSetExtern(float roll, float pitch, float yaw, float throttle, bool useThrottle);

#endif // __NORMAL_H
