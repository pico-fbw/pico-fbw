#ifndef __MODES_H
#define __MODES_H

#include <stdbool.h>

#define MODE_MIN MODE_DIRECT
typedef enum Mode {
    MODE_DIRECT,
    MODE_NORMAL,
    MODE_AUTO,
    MODE_TUNE,
    MODE_HOLD
} Mode;
#define MODE_MAX MODE_HOLD

/**
 * Transitions the system to a specified mode.
 * @param mode The mode to transition to.
*/
void toMode(Mode mode);

/**
 * Runs the code of the system's currently selected mode.
*/
void modeRuntime();

/**
 * @return The current mode of the system.
*/
Mode getCurrentMode();

/**
 * Declares whether or not the IMU data is safe to use.
*/
void setIMUSafe(bool state);

/**
 * Declares whether or not the GPS data is safe to use.
*/
void setGPSSafe(bool state);

#endif // __MODES_H
