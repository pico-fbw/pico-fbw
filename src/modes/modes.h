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
 * Transitions the system to a specified mode.
 * @param mode The mode to transition to.
*/
void toMode(uint8_t mode);

/**
 * Runs the code of the system's currently selected mode.
*/
void modeRuntime();

/**
 * Declares whether or not the IMU data is safe to use.
*/
void setIMUSafe(bool state);

#endif // modes_h
