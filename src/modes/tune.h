#ifndef __TUNE_H
#define __TUNE_H

/**
 * Executes tune mode.
*/
void mode_tune();

/**
 * Checks if the tuning mode has been run before.
 * @return true if calibration has been run previously, false if not.
*/
bool mode_tuneisCalibrated();

#endif // __TUNE_H
