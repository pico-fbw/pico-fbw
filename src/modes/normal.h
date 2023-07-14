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
 * @return true if the setpoints were adjusted, false if it was prevented by the user (already manually inputting).
*/
bool mode_normalSetSetpoints(float roll, float pitch, float yaw);

#endif // __NORMAL_H
