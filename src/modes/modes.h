#ifndef modes_h
#define modes_h

/**
 * Sets the current mode of the FBW system.
 * 0 - normal
 * 1 - direct
*/
void setMode(uint mode);

/**
 * Defines if the IMU data is safe to use or not.
*/
void setIMUSafe(bool state);

#endif // modes_h