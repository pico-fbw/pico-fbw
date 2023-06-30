#include <stdbool.h>

#ifndef modes_h
#define modes_h

// Define internal mode aliases
#define DIRECT 0
#define NORMAL 1
#define AUTO 2
#define TUNE 3
#define HOLD 4

/**
 * Sets the system into the specified mode.
 * Additionally, if the system is not already set into the specified mode, it will run the mode starting code.
*/
void mode(uint8_t smode);

/**
 * Declares whether or not the IMU data is safe to use.
*/
void setIMUSafe(bool state);

#endif // modes_h
