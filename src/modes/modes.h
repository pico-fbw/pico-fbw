#ifndef modes_h
#define modes_h

/**
 * Sets the current mode of the FBW system.
 * 0 - direct
 * 1 - normal
*/
void setMode(uint mode);

/**
 * Gets the current mode of the FBW system.
 * 0 - direct
 * 1 - normal
*/
uint getMode();

/**
 * Defines if the IMU data is safe to use or not.
*/
void setIMUSafe(int state);

#endif // modes_h
