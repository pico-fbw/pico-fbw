#ifndef __MODES_H
#define __MODES_H

#include <stdbool.h>

// Define internal mode aliases
#define DIRECT 0
#define NORMAL 1
#define AUTO 2
#define TUNE 3
#define HOLD 4
// If any modes are ever added, the uppermost mode must be added to the API!

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
 * @return The current mode of the system.
*/
uint8_t getCurrentMode();

/**
 * Declares whether or not the IMU data is safe to use.
*/
void setIMUSafe(bool state);

/**
 * Declares whether or not the GPS data is safe to use.
*/
void setGPSSafe(bool state);

#endif // __MODES_H
